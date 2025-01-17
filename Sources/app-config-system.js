/**
 * SPDX-FileCopyrightText: 2023-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium,
 * and 2018-2025 Open Health Imaging Foundation
 * SPDX-License-Identifier: MIT
 */

window.config.routerBasename = '${ROUTER_BASENAME}';

if (${USE_DICOM_WEB}) {
  window.config.dataSources = [
    {
      friendlyName: 'Orthanc DICOMweb',
      namespace: '@ohif/extension-default.dataSourcesModule.dicomweb',
      sourceName: 'dicomweb',
      configuration: {
        name: 'orthanc',

        wadoUriRoot: '../dicom-web',
        qidoRoot: '../dicom-web',
        wadoRoot: '../dicom-web',
        
        qidoSupportsIncludeField: false,
        supportsReject: false,
        imageRendering: 'wadors',
        thumbnailRendering: 'wadors',
        enableStudyLazyLoad: true,
        supportsFuzzyMatching: false,
        supportsWildcard: true,
        staticWado: true,
        singlepart: 'bulkdata',
        acceptHeader: [ 'multipart/related; type=application/octet-stream; transfer-syntax=*'],
        bulkDataURI: {  // to remove once 3.9.2+ is released (https://github.com/OHIF/Viewers/issues/4256)
          enabled: true
        }
      }
    }
  ];

  window.config.defaultDataSourceName = 'dicomweb';

} else {
  window.config.showStudyList = false;
  window.config.dataSources = [
    {
      friendlyName: 'Orthanc DICOM JSON',
      namespace: '@ohif/extension-default.dataSourcesModule.dicomjson',
      sourceName: 'dicomjson',
      configuration: {
        name: 'json',
      },
    }
  ];

  window.config.defaultDataSourceName = 'dicomjson';
}
