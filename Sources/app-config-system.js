/**
 * SPDX-FileCopyrightText: 2023 Sebastien Jodogne, UCLouvain, Belgium,
 * and 2018-2023 Open Health Imaging Foundation
 * SPDX-License-Identifier: MIT
 */

window.config.routerBasename = '${ROUTER_BASENAME}';

window.config.dataSources = [
  {
    friendlyName: 'Orthanc',
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
      singlepart: 'bulkdata'
    }
  }
];

window.config.defaultDataSourceName = 'dicomweb';
