#!/bin/bash
sudo mkdir ../efs
sudo mount -t nfs4 -o nfsvers=4.1,rsize=1048576,wsize=1048576,hard,timeo=600,retrans=2,noresvport fs-07976a37b3fd3d1f0.efs.us-east-1.amazonaws.com:/ ../efs
make
mkdir build
cp server ./build/server
sudo su