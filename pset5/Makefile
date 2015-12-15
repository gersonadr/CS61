DIRS	= uber-pi dispatch ubler

all:
	-for d in $(DIRS); do (cd $$d; $(MAKE) ); done

-include build/rules.mk

check:
	-for d in $(DIRS); do (cd $$d; $(MAKE) check); done

clean:
	-for d in $(DIRS); do (cd $$d; $(MAKE) clean); done
	rm -rf pset5.tgz pset5grade-aws.tgz

