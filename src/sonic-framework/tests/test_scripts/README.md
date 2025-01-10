  # Steps to run tests_script.sh

  1. Build the workspace, incase the workspace is not built using following commands.

     cd /sonic-buildiamge
   
     make init
   
     NOJESSIE=1 NOSTRETCH=1 NOBUSTER=1 NOBULLSEYE=1 SONIC_BUILD_JOBS=12 make configure PLATFORM=vs

     NOJESSIE=1 NOSTRETCH=1 NOBUSTER=1 NOBULLSEYE=1 SONIC_BUILD_JOBS=12  make target/sonic-vs.img.gz


  2. Create a virtual environment using following steps.
   
     Run create_virtualenv.sh script to create a virtual environment.
   
     sh create_virtualenv.sh ./


  3. Run test_script.sh inside the virtual envirment to execute the UT test cases

     sh test_script.sh ./

