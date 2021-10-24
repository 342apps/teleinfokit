# This script is executed before each build (extra_scripts in platformio.ini). 
# It calculates a md5 hash of folders /src (version.h is deleted before) and /data
# A six characters hash is then generated from the two folders hash concatenated.
# The file version.h is then created with compile time, build number, build hash and version

import datetime
import hashlib
import os
tm = datetime.datetime.today()

FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'include/version.h'
compileTime =  datetime.datetime.now()
version_root = 'v1.0.0.'
beta = '-beta1'
# beta = ''


def cheaphash(string,length=6):
    if length<len(hashlib.sha256(string.encode('utf-8')).hexdigest()):
        return hashlib.sha256(string.encode('utf-8')).hexdigest()[:length]
    else:
        raise Exception("Length too long. Length of {y} when hash length is {x}.".format(x=str(len(hashlib.sha256(string).hexdigest())),y=length))

def GetHashofDirs(directory, verbose=0):
  import hashlib, os
  SHAhash = hashlib.md5()
  if not os.path.exists (directory):
    return -1

  try:
    for root, dirs, files in os.walk(directory):
      for names in files:
        if verbose == 1:
          print('Hashing', names)
        filepath = os.path.join(root,names)
        try:
          f1 = open(filepath, 'rb')
        except:
          # You can't open the file for some reason
          f1.close()
          continue

        while 1:
          # Read file in as little chunks
          buf = f1.read(4096)
          if not buf : break
          varhash = hashlib.md5(buf)
          SHAhash.update(varhash.hexdigest().encode('utf-8'))
        f1.close()

  except:
    import traceback
    # Print the stack traceback
    traceback.print_exc()
    return -2

  return SHAhash.hexdigest()

build_no = 0

if os.path.exists(FILENAME_VERSION_H):
  os.remove(FILENAME_VERSION_H)

hashsrc = GetHashofDirs('src', 1)
hashdata = GetHashofDirs('data', 1)
globalhash = cheaphash(hashsrc + hashdata)
version = version_root+str(globalhash)+beta
print("==> Hash of source files: "+globalhash)

with open(FILENAME_BUILDNO, 'w+') as f:
    f.write(str(globalhash))
    print('Build number: {}'.format(build_no))
    print('Build hash: {}'.format(globalhash))
    print('Build time: {}'.format(compileTime))
    print('Version: {}'.format(version))

hf = """//  DO NOT EDIT MANUALLY THIS FILE - IT IS DELETED AND RECREATED AT EACH BUILD !!!! 
#ifndef BUILD_HASH
  #define BUILD_HASH "{}"
#endif
#ifndef BUILD_TIME
  #define BUILD_TIME "{}"
#endif
#ifndef VERSION
  #define VERSION "{}"
#endif
""".format(globalhash, compileTime, version)
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)