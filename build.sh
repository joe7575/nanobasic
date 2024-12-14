#!/bin/bash

sudo luarocks make

sudo luarocks remove nanobasic
sudo luarocks install nanobasic

