/**
 * SPDX-FileCopyrightText: 2023 Sebastien Jodogne, UCLouvain, Belgium
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * OHIF plugin for Orthanc
 * Copyright (C) 2023 Sebastien Jodogne, UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include <Logging.h>
#include <SystemToolbox.h>
#include <Toolbox.h>

#include <EmbeddedResources.h>

#include <boost/thread/shared_mutex.hpp>

// Forward declaration
void ReadStaticAsset(std::string& target,
                     const std::string& path);


/**
 * As the OHIF static assets are gzipped by the "EmbedStaticAssets.py"
 * script, we use a cache to maintain the uncompressed assets in order
 * to avoid multiple gzip decodings.
 **/
class ResourcesCache : public boost::noncopyable
{
private:
  typedef std::map<std::string, std::string*>  Content;
  
  boost::shared_mutex  mutex_;
  Content              content_;

public:
  ~ResourcesCache()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }

  void Answer(OrthancPluginContext* context,
              OrthancPluginRestOutput* output,
              const std::string& path)
  {
    const std::string mime = Orthanc::EnumerationToString(Orthanc::SystemToolbox::AutodetectMimeType(path));

    {
      // Check whether the cache already contains the resource
      boost::shared_lock<boost::shared_mutex> lock(mutex_);

      Content::const_iterator found = content_.find(path);
    
      if (found != content_.end())
      {
        assert(found->second != NULL);
        OrthancPluginAnswerBuffer(context, output, found->second->c_str(), found->second->size(), mime.c_str());
        return;
      }
    }

    // This resource has not been cached yet

    std::unique_ptr<std::string> item(new std::string);
    ReadStaticAsset(*item, path);
    OrthancPluginAnswerBuffer(context, output, item->c_str(), item->size(), mime.c_str());

    {
      // Store the resource into the cache
      boost::unique_lock<boost::shared_mutex> lock(mutex_);

      if (content_.find(path) == content_.end())
      {
        content_[path] = item.release();
      }
    }
  }
};


static ResourcesCache cache_;
static std::string    routerBasename_;

void ServeFile(OrthancPluginRestOutput* output,
               const char* url,
               const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();
  
  // The next 3 HTTP headers are required to enable SharedArrayBuffer
  // (https://web.dev/coop-coep/)
  OrthancPluginSetHttpHeader(context, output, "Cross-Origin-Embedder-Policy", "require-corp");
  OrthancPluginSetHttpHeader(context, output, "Cross-Origin-Opener-Policy", "same-origin");
  OrthancPluginSetHttpHeader(context, output, "Cross-Origin-Resource-Policy", "same-origin");

  std::string uri = request->groups[0];

  if (uri == "app-config.js")
  {
    std::string system, user;
    Orthanc::EmbeddedResources::GetFileResource(system, Orthanc::EmbeddedResources::APP_CONFIG_SYSTEM);
    Orthanc::EmbeddedResources::GetFileResource(user, Orthanc::EmbeddedResources::APP_CONFIG_USER);
    
    std::map<std::string, std::string> dictionary;
    dictionary["ROUTER_BASENAME"] = routerBasename_;

    system = Orthanc::Toolbox::SubstituteVariables(system, dictionary);

    std::string s = (user + "\n" + system);
    OrthancPluginAnswerBuffer(context, output, s.c_str(), s.size(), "application/json");
  }
  else if (uri == "viewer")
  {  
    cache_.Answer(context, output, "index.html");
  }
  else 
  {
    cache_.Answer(context, output, uri);
  }
}


OrthancPluginErrorCode OnChangeCallback(OrthancPluginChangeType changeType,
                                        OrthancPluginResourceType resourceType,
                                        const char* resourceId)
{
  try
  {
    if (changeType == OrthancPluginChangeType_OrthancStarted)
    {
      Json::Value info;
      if (!OrthancPlugins::RestApiGet(info, "/plugins/dicom-web", false))
      {
        throw Orthanc::OrthancException(
          Orthanc::ErrorCode_InternalError,
          "The OHIF plugin requires the DICOMweb plugin to be installed");
      }

      if (info.type() != Json::objectValue ||
          !info.isMember("ID") ||
          !info.isMember("Version") ||
          info["ID"].type() != Json::stringValue ||
          info["Version"].type() != Json::stringValue ||
          info["ID"].asString() != "dicom-web")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "The DICOMweb plugin is not properly installed");
      }
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception: " << e.What();
    return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
  }

  return OrthancPluginErrorCode_Success;
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    OrthancPlugins::SetGlobalContext(context);

    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context) == 0)
    {
      char info[1024];
      sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
              context->orthancVersion,
              ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      OrthancPluginLogError(context, info);
      return -1;
    }

#if ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(1, 7, 2)
    Orthanc::Logging::InitializePluginContext(context);
#else
    Orthanc::Logging::Initialize(context);
#endif

    OrthancPlugins::OrthancConfiguration configuration;

    {
      OrthancPlugins::OrthancConfiguration globalConfiguration;
      globalConfiguration.GetSection(configuration, "OHIF");
    }

    routerBasename_ = configuration.GetStringValue("RouterBasename", "/ohif");

    OrthancPluginSetDescription(context, "OHIF plugin for Orthanc.");

    OrthancPlugins::RegisterRestCallback<ServeFile>("/ohif/(.*)", true);

    OrthancPluginRegisterOnChangeCallback(context, OnChangeCallback);

    // Extend the default Orthanc Explorer with custom JavaScript for OHIF
    std::string explorer;
    Orthanc::EmbeddedResources::GetFileResource(explorer, Orthanc::EmbeddedResources::ORTHANC_EXPLORER);
    OrthancPluginExtendOrthancExplorer(context, explorer.c_str());

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "ohif";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_OHIF_VERSION;
  }
}
