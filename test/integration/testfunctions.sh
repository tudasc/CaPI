#!/bin/bash

#set path to capi
capi=
#set build dir/testexe
testExe=

#Function runs test
# Parameter 1: Path to the testcase (*.capi)
function runTestcase {
  testCaseFile=$1


  #set up diffrent data files:
  testFile=testCaseFile
  # groundtruth file (.gtjson)
  gtFile=${testCaseFile/capi/gtjson}
  # compile example.capi file --> example.json
  gFile=${testCaseFile/capi/json}
    #move to testfolder
    #compile with `capi -f example.capi --output-format json -o example.json lulesh_full.mcg`
    #move example.json to ./input...
    $capi -f testFile -output-format json -o gFile lulesh
  

  #execute tester on example.gtjson and example.json
  $testExe tFile gtFile
  #check if failure yes: return fail + 1  no: remove example.json (dont spam testcase folder)
  if [ $? -ne 0 ]; then
    echo "Failure for file: $gfile. Keeping generated file for inspection"
  else
    rm $gfile
  fi
  #return fail
  return $fail 
}