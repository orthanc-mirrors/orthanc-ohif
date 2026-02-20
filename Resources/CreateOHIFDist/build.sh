#!/bin/bash

# SPDX-FileCopyrightText: 2023-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
# SPDX-License-Identifier: GPL-3.0-or-later

# OHIF plugin for Orthanc
# Copyright (C) 2023-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

set -ex

if [ "$1" = "" ]; then
    echo "Please provide the source package of OHIF"
    exit -1
fi

# note: we really copy the build process from the OHIF Dockerfile.
mkdir /tmp/source/
cd /tmp/source/
tar xvf /source/$1.tar.gz

mkdir /tmp/$1
cd /tmp/source/$1

cp package.json yarn.lock preinstall.js lerna.json /tmp/$1/
cp --parents ./addOns/package.json ./addOns/*/*/package.json ./extensions/*/package.json ./modes/*/package.json ./platform/*/package.json /tmp/$1/

cd /tmp/$1
bun pm cache rm
bun install
bun add ajv@8.12.0

cd /tmp/source/$1
cp --link -r . /tmp/$1/ || true # this generates warnings but it is expected !

cd /tmp/$1
APP_CONFIG=config/default.js QUICK_BUILD=true PUBLIC_URL=./ bun run show:config
APP_CONFIG=config/default.js QUICK_BUILD=true PUBLIC_URL=./ bun run build

# patch files where the PUBLIC_URL was not taken into account
sed -i "s|var worker = new Worker(workerUrl|var worker = new Worker(new URL(window.location.protocol + '//' + window.location.host + window.location.pathname.replace('microscopy', 'dicom-microscopy-viewer') + '/dataLoader.worker.min.js')|g" /tmp/$1/platform/app/dist/dicom-microscopy-viewer/dicomMicroscopyViewer.min.js
sed -i 's|"/assets/android-chrome-|"./assets/android-chrome-|g' /tmp/$1/platform/app/dist/manifest.json

cp -r /tmp/$1/platform/app/dist/* /target
