#!/usr/bin/env python3

import argparse
import requests

parser = argparse.ArgumentParser(description = 'Clear the cache of the OHIF plugin (for "dicom-json" data source).')
parser.add_argument('--url',
                    default = 'http://localhost:8042',
                    help = 'URL to the REST API of the Orthanc server')
parser.add_argument('--username',
                    default = 'orthanc',
                    help = 'Username to the REST API')
parser.add_argument('--password',
                    default = 'orthanc',
                    help = 'Password to the REST API')

args = parser.parse_args()

auth = requests.auth.HTTPBasicAuth(args.username, args.password)

METADATA = '4202'

for instance in requests.get('%s/instances' % args.url, auth=auth).json():
    if METADATA in requests.get('%s/instances/%s/metadata' % (args.url, instance), auth=auth).json():
        requests.delete('%s/instances/%s/metadata/%s' % (args.url, instance, METADATA), auth=auth)
