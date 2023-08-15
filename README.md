#Masterthesis

This is the code repo of Ole Oggesens master thesis.

##Run instructions

To clone the repository and all its submodules run the following instructions.

```bash
git clone git@git.tu-berlin.de:masterthesis1/coderepo.git
git submodules init
git submodules update
```

The versions of dedup, created for this thesis are in the folder HLS. 
Follow the individual run instructions for respective dedup version.

##Structure

./HLS contains the dedup versions created for the argument of this thesis. 
./Software contains the dedup kernel of the PARSEC benchmark suite.
./Submodules contain the Vitis HLS libraries, out of which the sha1 kernel is taken.
