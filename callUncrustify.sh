#!/bin/bash
# executes uncrustify in the current folder on all .c and .h files
# only updates files which differ after the operation (avoiding tampering with file timestamps)

# parameters (for path specification, for instance)
UNCRUSTIFY_APP="uncrustify"
CONFIGURATION_FILE="uncrustify.cfg"

# variables
TEMP_FILE="___uncrustify_output___.tmp"
SILENT=0
NB_FILES_UNCRUSTIFIED=0
DEF_PATTERN='*.[ch]'
DEF_RPATTERN='*.[ch] ./**/*.[ch]'
FILE_PATTERN=$DEF_PATTERN

# process parameters
for i
do
 case "$i" in
  -c=*|--config=*)
   CONFIGURATION_FILE="${i#*=}"
   ;;
  -q|--quiet)
   SILENT=1
   ;;
  -r|--recursive)
   FILE_PATTERN=${DEF_RPATTERN}
   ;;
  -u=*|--uncrustify=*)
   UNCRUSTIFY_APP="${i#*=}"
   UNCRUSTIFY_APP="${UNCRUSTIFY_APP/.exe/uncrustify.exe}"
   ;;
  --help)
   echo 'uncrustify on .c and .h C files in the current folder'
   echo ' only modifed files are rewritten'
   echo 'optional parameters:'
   echo ' -c=... --config=...: defines configuration file to use'
   echo ' -q --quiet: does not display anything'
   echo ' -r --recursive: recursive process of files'
   echo ' -u=... --uncrustify=...: defines the uncrustify app to use'
   echo " \"<pattern>\": redefines file <pattern> instead \"${DEF_PATTERN}\" (\"${DEF_RPATTERN}\" if recursive)"
   echo "              caveat: surrounding double-quotes (\"...\") matter much!"
   exit
   ;;
  *)
   FILE_PATTERN=$i
   ;;
 esac
done

${UNCRUSTIFY_APP} --help >/dev/null 2>&1
if [[ $? -ne 0 ]]
then
 echo "failure: cannot run ${UNCRUSTIFY_APP} app"
 echo "it is possible to define the app with option -u=<uncrustifyApp>"
 echo "note: under Windows, it seems necessary to specify the '.exe' extension"
 echo "        and shortcut '-u=.exe' on command line adds this extension"
 exit
fi

# potential redirection of stdout
if [[ ${SILENT} -eq 1 ]]
then
 exec 1>/dev/null
fi

# the actual job
function doUncrust() {
 if [[ -f $1 ]]
 then
  ${UNCRUSTIFY_APP} -f $1 -c ${CONFIGURATION_FILE} -o ${TEMP_FILE} >/dev/null 2>&1
  diff --brief $1 ${TEMP_FILE} >/dev/null 2>&1
  if [[ $? -eq 0 ]]
  then
   rm ${TEMP_FILE}
  else
   echo $1 UNCRUSTIFIED! 2>&1
   mv ${TEMP_FILE} $1
   NB_FILES_UNCRUSTIFIED=$((NB_FILES_UNCRUSTIFIED + 1))
  fi
 else
  echo "$1 not found!" >/dev/null 2>&1
 fi
}

echo "Uncrustification started" 2>&1
shopt -s globstar
FILE_LIST="${FILE_PATTERN}"
for FILE in $FILE_LIST
 do
  doUncrust ${FILE}
 done
echo "Done - ${NB_FILES_UNCRUSTIFIED} file(s) uncrustified" 2>&1