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


$('#study').live('pagebeforeshow', function() {
  var studyId = $.mobile.pageData.uuid;

  $.ajax({
    url: '../studies/' + studyId,
    dataType: 'json',
    success: function(s) {
      var studyInstanceUid = s.MainDicomTags.StudyInstanceUID;
      
      $('#ohif-button').remove();

      var b = $('<a>')
          .attr('id', 'ohif-button')
          .attr('data-role', 'button')
          .attr('href', '#')
          .attr('data-icon', 'search')
          .attr('data-theme', 'e')
          .text('Open OHIF viewer')
          .button();
      
      b.insertAfter($('#study-info'));
      b.click(function() {
        if (${USE_DICOM_WEB}) {
          window.open('../ohif/viewer?StudyInstanceUIDs=' + studyInstanceUid);
        } else  {
          window.open('../ohif/viewer?url=../ohif-source/' + studyId);
        }
      });
    }
  });
});


if (${USE_DICOM_WEB}) {
  $('#lookup').live('pagebeforeshow', function() {
    $('#open-ohif-study-list').remove();
    
    var b = $('<fieldset>')
        .attr('id', 'open-ohif-study-list')
        .addClass('ui-grid-b')
        .append($('<div>')
                .addClass('ui-block-a'))
        .append($('<div>')
                .addClass('ui-block-b')
                .append($('<a>')
                        .attr('id', 'coucou')
                        .attr('data-role', 'button')
                        .attr('href', '#')
                        .attr('data-icon', 'forward')
                        .attr('data-theme', 'a')
                        .text('Open OHIF study list')
                        .button()
                        .click(function(e) {
                          window.open('../ohif/');
                        })));
    
    b.insertAfter($('#lookup-result'));
  });
}
