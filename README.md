roerei
======
Premise selection for Coq in OCaml and C++.

Installing dependencies
-----------------------
Build the docker image by running:
```$ docker build . -t roerei```

Enter the image by running in the git project folder:
```$ docker run -it -v `pwd`:/app roerei bash```

You will now have downloaded and installed all necessary dependencies to run this project
within the docker container.

Preparation
-----------
You first need to get a repository with the relevant Coq corpus precompiled.
For any corpus you can do this by building with:

```COQ_XML=-xml COQ_XML_LIBRARY_ROOT=<dest>```

Depending on the setup of the corresponding Makefile, the Coq compiler `coqc`
will emit XML-files alongside the `.vo` object files.
You can also download the compiled XML as used in this thesis by contacting the author. (approximately 6.9 GiB)

You can now put your desired repository in the `src/preloader` folder by symlinking at `src/preloader/repo`.
Ensure that the actual repository files are accessible from within the Docker-container.

```
# cd /app/src/preloader
# ln -s <repo path> repo
# ./build.sh
# ./preloader
```

You will now have created the `repo.msgpack` (207 MiB) and `mapping.msgpack` (649 KiB) files.
These files contain the summarized contents of the XML repository, and will be used to generate the actual datasets.
Put these files in the `/app/data` folder.

```
# cp -t /app/data repo.msgpack mapping.msgpack
```

Finally we will need to build `roerei` itself.

```
# cd /app
# ./cmake-linux.sh
# cd /app/build
# make -j<cpu-count>
```

How to run
----------
First, check out the help:

```
# ./src/roerei/roerei -h

Premise selection for Coq in OCaml/C++. [https://github.com/Wassasin/roerei]
Usage: ./roerei [options] action

Actions:
  generate                 load repo.msgpack, convert and write to dataset.msgpack
  dump                     load repo.msgpack, and dump its contents in human readable format to repo.txt
  inspect                  inspect all objects
  measure                  run all scheduled tests and store the results
  report [results]         report on all results [in file 'results']
  diff <c1> <c2>           show the difference between two corpii
  export [results] <dest>  dump all results [from file 'results-path'] into predefined format for thesis in directory <path>
  upgrade <src>						 upgrade non-prior results dataset to newest version
  legacy-export            export dataset in the legacy format
  legacy-import            import dataset in the legacy format

Options:
  -h [ --help ]         display this message
  -s [ --silent ]       do not print progress
  -P [ --no-prior ]     do not use prior datasets, if applicable
  -C [ --no-cv ]        do not use crossvalidation, if applicable
  -c [ --corpii ] arg   select which corpii to sample or generate, possibly 
                        comma separated (default: all)
  -m [ --methods ] arg  select which methods to use, possibly comma separated 
                        (default: all)
  -r [ --strats ] arg   select which poset consistency strategies to use, 
                        possibly comma separated (default: all)
  -j [ --jobs ] arg     number of concurrent jobs (default: 1)
  -f [ --filter ] arg   show only objects which include the filter string
```

Secondly, generate the actual datasets:
```
# ./src/roerei/roerei generate
```

This will emit various files for each corpus.
Now perform your performance measurements on a beefy machine:
```
# ./src/roerei/roerei measure -j<num-cpu>
```

Finally export your performance results as various TeX-files:
```
# mkdir ./results
# ./src/roerei/roerei export ./results
```