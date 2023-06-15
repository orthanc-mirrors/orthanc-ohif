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

#include <DicomFormat/DicomMap.h>
#include <Logging.h>
#include <SystemToolbox.h>
#include <Toolbox.h>

#include <EmbeddedResources.h>

#include <boost/thread/shared_mutex.hpp>


// Reference: https://v3-docs.ohif.org/configuration/dataSources/dicom-json

enum DataType
{
  DataType_String,
  DataType_Integer,
  DataType_Float,
  DataType_ListOfFloats,
  DataType_ListOfStrings,
  DataType_None
};

class TagInformation
{
private:
  DataType     type_;
  std::string  name_;
  
public:
  TagInformation() :
    type_(DataType_None)
  {
  }
  
  TagInformation(DataType type,
                 const std::string& name) :
    type_(type),
    name_(name)
  {
  }

  DataType GetType() const
  {
    return type_;
  }

  const std::string& GetName() const
  {
    return name_;
  }

  bool operator== (const TagInformation& other) const
  {
    return (type_ == other.type_ &&
            name_ == other.name_);
  }
};

typedef std::map<Orthanc::DicomTag, TagInformation>  TagsDictionary;

static TagsDictionary ohifStudyTags_, ohifSeriesTags_, ohifInstanceTags_, allTags_;

static void InitializeOhifTags()
{
  ohifStudyTags_[Orthanc::DICOM_TAG_STUDY_INSTANCE_UID] = TagInformation(DataType_String, "StudyInstanceUID");
  ohifStudyTags_[Orthanc::DICOM_TAG_STUDY_DATE]         = TagInformation(DataType_String, "StudyDate");
  ohifStudyTags_[Orthanc::DICOM_TAG_STUDY_TIME]         = TagInformation(DataType_String, "StudyTime");
  ohifStudyTags_[Orthanc::DICOM_TAG_PATIENT_NAME]       = TagInformation(DataType_String, "PatientName"); 
  ohifStudyTags_[Orthanc::DICOM_TAG_PATIENT_ID]         = TagInformation(DataType_String, "PatientID");
  ohifStudyTags_[Orthanc::DICOM_TAG_ACCESSION_NUMBER]   = TagInformation(DataType_String, "AccessionNumber");
  ohifStudyTags_[Orthanc::DicomTag(0x0010, 0x1010)]     = TagInformation(DataType_String, "PatientAge");
  ohifStudyTags_[Orthanc::DICOM_TAG_PATIENT_SEX]        = TagInformation(DataType_String, "PatientSex");

  ohifSeriesTags_[Orthanc::DICOM_TAG_SERIES_INSTANCE_UID] = TagInformation(DataType_String, "SeriesInstanceUID");
  ohifSeriesTags_[Orthanc::DICOM_TAG_SERIES_NUMBER]       = TagInformation(DataType_Integer, "SeriesNumber");
  ohifSeriesTags_[Orthanc::DICOM_TAG_MODALITY]            = TagInformation(DataType_String, "Modality");
  ohifSeriesTags_[Orthanc::DICOM_TAG_SLICE_THICKNESS]     = TagInformation(DataType_Float, "SliceThickness");

  ohifInstanceTags_[Orthanc::DICOM_TAG_COLUMNS]                    = TagInformation(DataType_Integer, "Columns");
  ohifInstanceTags_[Orthanc::DICOM_TAG_ROWS]                       = TagInformation(DataType_Integer, "Rows");
  ohifInstanceTags_[Orthanc::DICOM_TAG_INSTANCE_NUMBER]            = TagInformation(DataType_Integer, "InstanceNumber");
  ohifInstanceTags_[Orthanc::DICOM_TAG_SOP_CLASS_UID]              = TagInformation(DataType_String, "SOPClassUID");
  ohifInstanceTags_[Orthanc::DICOM_TAG_PHOTOMETRIC_INTERPRETATION] = TagInformation(DataType_String, "PhotometricInterpretation");
  ohifInstanceTags_[Orthanc::DICOM_TAG_BITS_ALLOCATED]             = TagInformation(DataType_Integer, "BitsAllocated");
  ohifInstanceTags_[Orthanc::DICOM_TAG_BITS_STORED]                = TagInformation(DataType_Integer, "BitsStored");
  ohifInstanceTags_[Orthanc::DICOM_TAG_PIXEL_REPRESENTATION]       = TagInformation(DataType_Integer, "PixelRepresentation");
  ohifInstanceTags_[Orthanc::DICOM_TAG_SAMPLES_PER_PIXEL]          = TagInformation(DataType_Integer, "SamplesPerPixel");
  ohifInstanceTags_[Orthanc::DICOM_TAG_PIXEL_SPACING]              = TagInformation(DataType_ListOfFloats, "PixelSpacing");
  ohifInstanceTags_[Orthanc::DICOM_TAG_HIGH_BIT]                   = TagInformation(DataType_Integer, "HighBit");
  ohifInstanceTags_[Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT]  = TagInformation(DataType_ListOfFloats, "ImageOrientationPatient");
  ohifInstanceTags_[Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT]     = TagInformation(DataType_ListOfFloats, "ImagePositionPatient");
  ohifInstanceTags_[Orthanc::DICOM_TAG_FRAME_OF_REFERENCE_UID]     = TagInformation(DataType_String, "FrameOfReferenceUID");
  ohifInstanceTags_[Orthanc::DicomTag(0x0008, 0x0008)]             = TagInformation(DataType_ListOfStrings, "ImageType");
  ohifInstanceTags_[Orthanc::DICOM_TAG_MODALITY]                   = TagInformation(DataType_String, "Modality");
  ohifInstanceTags_[Orthanc::DICOM_TAG_SOP_INSTANCE_UID]           = TagInformation(DataType_String, "SOPInstanceUID");
  ohifInstanceTags_[Orthanc::DICOM_TAG_SERIES_INSTANCE_UID]        = TagInformation(DataType_String, "SeriesInstanceUID");
  ohifInstanceTags_[Orthanc::DICOM_TAG_STUDY_INSTANCE_UID]         = TagInformation(DataType_String, "StudyInstanceUID");
  ohifInstanceTags_[Orthanc::DICOM_TAG_WINDOW_CENTER]              = TagInformation(DataType_Float, "WindowCenter");
  ohifInstanceTags_[Orthanc::DICOM_TAG_WINDOW_WIDTH]               = TagInformation(DataType_Float, "WindowWidth");
  ohifInstanceTags_[Orthanc::DICOM_TAG_SERIES_DATE]                = TagInformation(DataType_String, "SeriesDate");

  for (TagsDictionary::const_iterator it = ohifStudyTags_.begin(); it != ohifStudyTags_.end(); ++it)
  {
    assert(allTags_.find(it->first) == allTags_.end() ||
           allTags_[it->first] == it->second);
    allTags_[it->first] = it->second;
  }

  for (TagsDictionary::const_iterator it = ohifSeriesTags_.begin(); it != ohifSeriesTags_.end(); ++it)
  {
    assert(allTags_.find(it->first) == allTags_.end() ||
           allTags_[it->first] == it->second);
    allTags_[it->first] = it->second;
  }

  for (TagsDictionary::const_iterator it = ohifInstanceTags_.begin(); it != ohifInstanceTags_.end(); ++it)
  {
    assert(allTags_.find(it->first) == allTags_.end() ||
           allTags_[it->first] == it->second);
    allTags_[it->first] = it->second;
  }
}


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


static void GetOhifDicomTags(Json::Value& target,
                             const std::string& instanceId)
{
  Json::Value source;
  if (!OrthancPlugins::RestApiGet(source, "/instances/" + instanceId + "/tags?short", false))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
  }

  if (source.type() != Json::objectValue)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }

  for (TagsDictionary::const_iterator it = allTags_.begin(); it != allTags_.end(); ++it)
  {
    const std::string tag = it->first.Format();
    
    if (source.isMember(tag))
    {
      const Json::Value& value = source[tag];

      /**
       * The cases below derive from "Toolbox::SimplifyDicomAsJson()"
       * with "DicomToJsonFormat_Short", which is invoked by the REST
       * API call to "/instances/.../tags?short".
       **/

      switch (value.type())
      {
        case Json::nullValue:
          break;
          
        case Json::arrayValue:
          // This should never happen, as this would correspond to a sequence
          break;

        case Json::stringValue:
        {
          switch (it->second.GetType())
          {
            case DataType_String:
              target[tag] = value;
              break;

            case DataType_Integer:
              target[tag] = boost::lexical_cast<int>(value.asString());
              break;

            case DataType_Float:
              target[tag] = boost::lexical_cast<float>(value.asString());
              break;

            default:
              // TODO
              break;
          }
        }

        default:
          // This should never happen
          break;
      }
    }
  }
  
  //Orthanc::DicomMap m;
  
}


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
      /*{
        Json::Value v;
        GetOhifDicomTags(v, "4368e7c1-33f7303d-5fc9fcc6-6e5fde31-6959b209");
        std::cout << v.toStyledString();
        }*/
      
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

    InitializeOhifTags();

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
