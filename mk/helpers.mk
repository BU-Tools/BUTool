#################################################################################
# make stuff
#################################################################################
SHELL=/bin/bash -o pipefail

#add path so build can be more generic
#
#MAKE_PATH ?= $(shell dirname $(realpath ./))
MAKE_PATH ?= $(realpath ./)

OUTPUT_MARKUP= 2>&1 | tee -a ../make_log.txt | ccze -A

init:
	git submodule update --init --recursive 

git_remote_to_ssh:
	@git remote -v | grep push | sed -e 's/https:\/\//git@/g' -e 's/.com\//.com:/g' -e 'i\git remote set-url --push ' -e 's/(push)//' | xargs | bash
	@git submodule foreach `git remote -v | grep push | sed -e 's/https:\/\//git@/g' -e 's/.com\//.com:/g' -e 'i\git remote set-url --push ' -e 's/(push)//' | xargs`
git_remote_to_https:
	@git remote -v | grep push | sed -e 's/git@/https:\/\//g' -e 's/.com:/.com\//g' -e 'i\git remote set-url --push ' -e 's/(push)//' | xargs | bash
	@git submodule foreach `git remote -v | grep push | sed -e 's/git@/https:\/\//g' -e 's/.com:/.com\//g' -e 'i\git remote set-url --push ' -e 's/(push)//' | xargs `

#################################################################################
# gcc fallthrough checking
#################################################################################
-include fallthrough.mk

#################################################################################
# Help 
#################################################################################
#list magic: https://stackoverflow.com/questions/4219255/how-do-you-get-the-list-of-targets-in-a-makefile
#define that builds a list of make rules based on a regex
#ex:
#   $(call LIST_template,REXEX)
define LIST_template = 
@$(MAKE) -pRrq -f $(MAKEFILE_LIST) | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | { grep $(1) || true;} | { grep -v prebuild || true;} | { egrep -v -e '^[^[:alnum:]]' -e '^$@$$' || true;} | column
endef

list:
	$(call LIST_template,.)

git_log:
	git log --graph --full-history --all --color --pretty=format:"%x1b[31m%h%x09%x1b[32m%d%x1b[0m%x20%s"
