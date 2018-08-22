#!/bin/bash
if [ "$#" -ne 4 ]
	then echo "Missing a mandatory flag"
	exit 1
fi

while getopts l:c: option
do
    case "${option}"
    in
        l) DIR=${OPTARG};;
        c) COM=${OPTARG};;
    esac
done

if [ -d "$DIR" ]; then
	if [[ "${COM}" = "list" ]]; then
		cd "${DIR}"
		ls -d -l */
	elif [[ "${COM}" = "size" ]]; then
		du -h --apparent-size --max-depth=1 "${DIR}" | sort -n -r
	elif [[ "${COM}" = "purge" ]]; then
		rm -r -f ${DIR}
		echo Deleted all folders
	fi
else
	echo No such folder exists.
fi
