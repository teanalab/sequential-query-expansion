#!/usr/bin/env python

#	Copyright 2013 AlchemyAPI
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


from __future__ import print_function
from alchemyapi import AlchemyAPI
import json
import sys


text = sys.argv[1]

print ("text =", text)

alchemyapi = AlchemyAPI()

response = alchemyapi.combined('text', text)

if response['status'] == 'OK':
    print('## Response Object ##')
    print(json.dumps(response, indent=4))

    print('')

    print('## Keywords ##')
    for keyword in response['keywords']:
        print(keyword['text'], ' : ', keyword['relevance'])
    print('')

    print('## Concepts ##')
    for concept in response['concepts']:
        print(concept['text'], ' : ', concept['relevance'])
    print('')

    print('## Entities ##')
    for entity in response['entities']:
        print(entity['type'], ' : ', entity['text'], ', ', entity['relevance'])
    print(' ')

else:
    print('Error in combined call: ', response['statusInfo'])

