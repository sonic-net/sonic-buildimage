#!/bin/bash
basedir=`dirname $0`

cd $basedir
basedir=`pwd`
cd - 1> /dev/null

function usage
{
  echo ""
  echo "run_test.sh - Run tests on Sonic DUT"
  echo "Usage: "
  echo "   $0 [OPTIONS] "
  echo " "
  echo " OPTIONS: "
  echo "    -t | --target_box     target box defined in westford_hw_inventory file in sonic-mgmt/ansible/ directory - required if not using '-r' gitlab opion"
  echo "    -i | --image_url      image where the sonic-broadcom.bin or docker-sonic-minion.gz is located"
  echo "    --topology            Topology to run tests on - supported are 'express, t0-52, t0, t1'. Default is 'express'"
  echo "    --suite               Suite to run - supported are 'express'. Default is 'express'"
  echo "    --powercycle          Powercylce the 'target' box before running tests"
  echo "    --no_cleanup          Don't cleanup, leave things setup for further debugging"
  echo "    --cleanup             cleanup the topology without running any tests."
  echo "    --upgrade             Perform upgrade"
  echo "                              For minion - this will stop an existing running docker and load the docker image"
  echo "                              For h/w - this will do sonic-installer install of the image"
  echo "    --configure          Based on topology and target box, generate config files"
  echo "                              For minion - this will start the minion dockers"
  echo "                              For h/w - this will scp the files to /etc/sonic and reboot the target box and validate healthcheck"
  echo "    --test               Run pytest tests for the specified topology against the target box"
  echo "    --no_reserve         Do not add reservation for target box via altos"
  echo "    --minion_docker_tag  Tag on the minion docker image to load - you can use this if you have already loaded the docker image and thus don't need to specify --upgrade option"
  echo "    --notify             sends email provide a list of receipents for e.g 'usr1,usr2,..', default is 'none', 'db' will send to regressdb"
  echo "    --run_add_topo       run add-topo only and if --configure option is not specified"
  echo "    --run_health_check   run health_check after reboot/upgrade, required only if --configure option is not specified"
  echo "    --run_in_order       run pytest tests in order that they are specified"
  echo "    --ndk_deb_image_url  (url) path to the NDK deb image to use for testing."
  echo ""
  echo " OPTIONS used by gitlab ci/cd process"
  echo "    -g | --git_clone_str       git clone string to use for cloning sonic-mgmt git repo - this is needed for gitlab"
  echo "    -r | --gitlab_runner_name  name/description of the gitlab_runner choses to run tests"
  echo "                                  we will choose the target based on the runner name from .gitlab-express-runner.conf file"
  echo ""
  echo "   Example:"
  echo "     # Run express suite to upgrade only to http://152.148.151.183/files/sonic-broadcom.bin on ixr_pizza_1 target box"
  echo "          ./run_test.sh -t ixr_pizza_1 -i http://152.148.151.183/files/sonic-broadcom.bin --upgrade"
  echo "     # Run express suite with upgrade the box, setup/configure for $topology, and runs pytests"
  echo "          ./run_test.sh -t ixr_hr_board2 -i target/sonic-broadcom.bin --upgrade --configure --test"
  echo "     # Run express suite without any upgrade, but configure and run pytests on ixr_hr_board2 target box"
  echo "          ./run_test.sh -t ixr_hr_board2 -i target/sonic-broadcom.bin --configure --test"
  echo "     # Run express suite (pytests) only with no upgrade and no configure - setup is ready to be tested"
  echo "          ./run_test.sh -t ixr_hr_board2 -i target/sonic-broadcom.bin --test"
}

target_box=""
image_url=''
ndk_image_url=''
topology='express'
suite='express'
notify=''

gitclone_str='git clone git@gitlabsr.nuq.ion.nokia.net:'
gitlab_runner_name=''
using_gitlab=0
upgrade=0
configure=0
test=0
no_cleanup=0
cleaup_minion_only=0
powercycle=0
docker_tag=''
run_add_top=0
run_health_check=0
run_in_order=true
no_reservation=0
post_results=0


function cleanup()
{
  test_failed=$1
  cancel_reservation

  if [[ "$no_cleanup" == "1" ]]; then
    return
  fi

  if [ ! -z ${vm_type+x} ]; then
    echo "Dealing with fanout topology, lets bring down the simulated DUT's using remove-topo and stop-vms"
    remove_topo $target_box $testbed_file $inv_file $vm_type $conf_name $topo_builder_dir/results/log/remove-topo.log
    stop_vms $target_box $testbed_file $inv_file $topo_builder_dir/results/log/stop-vms.log
    $minion_dir/topo_builder/topology_builder.py $topology $target_box $topo_builder_dir action=teardown_testbed_server log_level=WARNING
  fi

  stop_sonic_mgmt_docker $target_box
  
  if [ ! -z ${minion_docker_name+x} ]; then
    if [[ "$(docker ps -a | grep $minion_docker_name)" ]]; then
      echo "Stopping existing docker '$minion_docker_name'"
      $minion_dir/topo_builder/topology_builder.py $topology $target_box $topo_builder_dir action=teardown log_level=WARNING
    fi
  fi
  
}

function run_add_topo()
{
  if [ ! -z ${vm_type+x} ]; then    
    echo "Dealing with fanout topology, lets bring up the simulated DUTs"
    if [ "${target_box}" == "vms-kvm" ]; then
  	  setup_testbed_server $target_box $topology $topo_builder_dir sonic_vs_target=$image_url
    elif [ "${target_box}" == "vms-kvm-t2" ]; then
	  setup_testbed_server $target_box $topology $topo_builder_dir sonic_vs_target=$image_url
      return
    else
      setup_testbed_server $target_box $topology $topo_builder_dir
    fi

    if [[ "$conf_name" == "ixre-chassis11-t2" || "$conf_name" == "ixre-chassis9-t2" ]]; then
      echo "Sleeping for 6 minute as we are not creating VMs"
      sleep 360  
      return
    fi
    add_topo $target_box $testbed_file $inv_file $vm_type $conf_name $topo_builder_dir/results/log/add-topo.log

    echo "Sleeping for 90 seconds for simulated DUTs to start"
    sleep 90

    inv_file_name=`basename $inv_file`
    testbed_file_name=`basename $testbed_file`

    check_ssh_to_vms $sonic_mgmt_dir/ansible/$inv_file_name $sonic_mgmt_dir/ansible/$testbed_file_name $target_box $topology
  fi
}

function run_test_exit
{ 
  code=$1

  cleanup $code
  exit $code
}
    


while [ "$1" != "" ]; do
    case $1 in
        -g | --git_clone_str ) shift
                               gitclone_str="git clone $1/"
                               using_gitlab=1
                             ;;
        -i | --image_url )   shift
                             image_url=$1
                             ;;
        -r | --gitlab_runner_name )  shift
                             gitlab_runner_name=$1
                             ;;
        -t | --target_box )  shift
                             target_box=$1
                             ;;
        --topology )         shift
                             topology=$1
                             ;;
        --suite )            shift
                             suite=$1
                             ;;
        --no_cleanup )       no_cleanup=1
                             ;;
        --cleanup )          cleanup_only=1
                             ;;
        --upgrade )          upgrade=1
                             ;;
        --configure )        configure=1
                             ;;
        --test )             test=1
                             ;;
        --minion_docker_tag ) shift
                             docker_tag=$1
                             ;;
        --no_reserve )       no_reservation=1
                             ;;
        --powercycle )       powercycle=1
                             ;;
        --run_add_topo )     run_add_topo=1
                             ;;
        --run_health_check ) run_health_check=1
                             ;;
        --run_in_order )     shift
                             run_in_order=$1
                             ;;
        --notify )           shift
                             notify=$1
                             ;;
        --post_results )     post_results=1
                             ;;
        --ndk_deb_image_url ) shift
                              ndk_image_url=$1
                              ;;
        * )                  usage
                             exit 1
    esac
    shift
done

if [[ $using_gitlab == 1 ]]; then
    notify='db'
else
    if [[ $notify == '' ]]; then
        notify='none'
    fi
fi

if [[ "$gitlab_runner_name" != "" ]]; then
  # We are using gitlab, we need to pick the target based on .gitlab-express-runner.conf
  echo "Using '$gitlab_runner_name' overwritting target box from $basedir/.gitlab-express-runner.conf file"
  source $basedir/.gitlab-express-runner.conf
  target_box="${!gitlab_runner_name}"
fi

if [[ "$target_box" == "" ]]; then
  if [[ "$gitlab_runner_name" == "" ]]; then
    echo "No target box specified. Either use \"-t\" option or for ci/cd use \"-r\" option"
  else
     echo "No corresponding target found for gitlab runner '$gitlab_runner_name'"
  fi
  exit 1
fi

altos_dir=$basedir/altos
regress_scripts_dir=$altos_dir/regress_scripts

source $regress_scripts_dir/altos_utils.sh
source $regress_scripts_dir/pc_utils.sh
source $regress_scripts_dir/dut_utils.sh
source $regress_scripts_dir/regress_db_utils.sh
source $regress_scripts_dir/minion_utils.sh
source $regress_scripts_dir/topo_builder_utils.sh

minion_dir=$basedir/altos/minion
sonic_mgmt_dir=$basedir/src/sonic-mgmt

output_dir=$basedir/output_files
mkdir -p $output_dir

dump_dir=$output_dir/$target_box/$topology/results/dut_dumps
mkdir -p $dump_dir
chmod 777 $dump_dir

pytest_results_dir=$output_dir/$target_box/$topology/results
sudo rm -fr $pytest_results_dir
mkdir -p $pytest_results_dir


# Make topo_builder for output for topo_builder
topo_builder_dir=$output_dir/$target_box/$topology

if [[ $target_box == "minion"* ]]; then
  minion_docker_name="express-dut1-imm1"
  if [[ "$target_box" == "minion_chassis" ]]; then
    minion_docker_name="poc-express-dut1-imm1"
  fi
  if  [ -d $topo_builder_dir/runtime ]; then
    # cleanup runtime dir under topo_builder_dir as this is where the netns for minion go
    if [[ "$upgrade" == "0" && "$configure" == "0" ]] ; then
      echo "Not upgrading/configuring - so not deleting existing runtime dir"
   else
      echo "runtime dir exits - deleting it"
      sudo rm -fr $topo_builder_dir/runtime
    fi
  fi
fi

mkdir -p $HOME/.ssh
mkdir -p $topo_builder_dir
mkdir -p $topo_builder_dir/results/log

# Install ovs
install_ovs

uid=`id -u`
gid=`id -g`

echo "Running with topology '$topology' on target '$target_box'"
conf_name="$target_box-$topology"

if [ "$upgrade" == "1" ] || [ "${target_box}" == "vms-kvm" ]; then
  validate_image_path $image_url
fi

if [ "$ndk_image_url" != "" ]; then
  validate_image_path $ndk_image_url
fi


echo "Killing existing altos docker for $target_box"
stop_altos_docker $target_box

if [[ $using_gitlab == 1 ]]; then     
  check_is_reservable='False'
else
  check_is_reservable='True'
fi  

if [[ ("$no_reservation" == "0" && $target_box != "minion"*) && ("$no_reservation" == "0" && $target_box != "vms-kvm"*) ]]; then
  if [[ "$topology" == "t2" ]]; then
      add_reservation $target_box 3000 $topology $check_is_reservable        
  else
    add_reservation $target_box 120 $topology $check_is_reservable
  fi
fi

echo "Running 'sudo pip install -q ---proxy=http://proxy.lbs.alcatel-lucent.com:8000 -r $minion_dir/topo_builder/requirements.txt"
sudo pip install -q --proxy=http://proxy.lbs.alcatel-lucent.com:8000 -r $minion_dir/topo_builder/requirements.txt
if [ "$?" != "0" ]; then
  echo "Could not install the required pip packages in '$minion_dir/topo_builder/requirements.txt' to run topology builder"
  exit 1
fi

# Powercycle the target_box if required
if [[ "$powercycle" == "1" ]]; then
  powercycle_dut $target_box
fi

generated_file_dir="$topo_builder_dir/results/generated_files/"



# Generate required files like configs/connection graph
gen_cfg_files $target_box $topology $topo_builder_dir

# this will generate all the vars that are needed to do orchestration and run tests.
echo "sourcing runtime vars defined in '$generated_file_dir/runtime_vars'"
source $generated_file_dir/runtime_vars


# start the sonic-mgmt docker
#docker_map_str="-v $sonic_mgmt_dir:/data -v $dump_dir:/dump_dir -v $pytest_results_dir:/output_files -v $generated_file_dir:/generated_files"
start_sonic_mgmt_docker $target_box "$sonic_mgmt_dir" "$pytest_results_dir"

if [[ "$cleanup_only" == "1" ]]; then
  echo "Cleaning up the topology '$topology' and target box '$target_box'"
  run_test_exit 0
fi

# cleanup sonic images on the target box if not minion
# this is to avoid issue where sometimes files transfered via 'scp' get overwritten when image to upgrade is already available on the box.
if [[ "$upgrade" == "1" && $target_box != *"minion"* && $target_box != "vms-kvm"* ]]; then
  echo "cleaning up images"
  cleanup_sonic_images $target_box $topo_builder_dir/results/log
fi

# generate args file for regress_db
prerun_regress_db $notify $topology $pytest_results_dir $basedir

if [[ "$upgrade" == "1" ]]; then
  if [[ $target_box == *"minion"* ]]; then
    upgrade_minion $docker_tag $image_url $target_box
  else
    upgrade_dut $target_box $image_url $topo_builder_dir/results/log
    if [[ $target_box == *"ixre"* ]]; then
      echo "Sleeping for 2 more minutes for mgmt connectivity to come up on ixre boards"
      sleep 120   
    else
      echo "Sleeping for 9 more minutes for phyMgr to be done and SONIC to stabalize"
      sleep 660 
    fi
  fi
fi

if [[ "$configure" == "1" ]]; then
  if [[ $target_box == *"minion"* ]]; then
    configure_minion $docker_tag $minion_docker_name $target_box $topology $topo_builder_dir
    run_add_topo    
  else
    gen_mg_successful=1
    # run gen-mg and deploy the configuration
    for i in 1 2 3 4
    do               
      echo "Generating and loading minigraph on $target_box in try number $i - log will be at $topo_builder_dir/results/log/gen-mg-$i.log"  
      docker exec -t  --user="$uid:$gid" -w /data/ansible sonic_mgmt_$target_box ./testbed-cli.sh -t $testbed_file -m $inv_file gen-mg $conf_name $lab_file password.txt -e deploy=true -e save=true -vvv > $topo_builder_dir/results/log/gen-mg-$i.log 2>&1
      if [[ "$?" == "0" ]]; then
        echo "Successfully generated minigraph and loaded in on '$target_box' for topology 'topology' in try number $i"
        gen_mg_successful=0
        break  
      else
        echo "Generating minigraph of topology '$topology' on '$target_box' did not pass in try number $i"
      fi
    done

    if [[ $gen_mg_successful == 1 ]]; then
      echo "Generating minigraph of topology '$topology' on '$target_box' did not pass in 4 tries"
      run_test_exit 1
    fi
    
    # make sure things are up and
    docker exec -t -w /data/ansible sonic_mgmt_$target_box ./run_playbook.sh -p healthcheck.yml -t $target_box -e "dump_dir='/dump_dir'"
    if [[ "$?" == "0" ]]; then
      echo "Configure/setup of topology '$topology' on '$target_box' after reboot with '$image_url' passed"
    else
      echo "Configure/setup of topology '$topology' on '$target_box' after reboot with '$image_url' did not pass"
      run_test_exit 1
    fi
    run_add_topo
  fi
fi

if [[ "$run_add_topo" == "1" && "$configure" != "1"  ]]; then
  run_add_topo
fi

if [[ "$run_health_check" == "1" && "$configure" != "1" ]]; then
  # make sure things are up and
  docker exec -t -w /data/ansible sonic_mgmt_$target_box ./run_playbook.sh -p healthcheck.yml -t $target_box -e "dump_dir='/dump_dir'"
  if [[ "$?" == "0" ]]; then
    echo "Configure/setup of topology '$topology' on '$target_box' after reboot with '$image_url' passed"
  else
    echo "Configure/setup of topology '$topology' on '$target_box' after reboot with '$image_url' did not pass"
    run_test_exit 1
  fi
fi

if [[ "$test" == "1" ]]; then
  if [[ "$test_type" == "OC" ]]; then
    if [[ "$dev_type" == "vs" ]]; then
      echo "Executing docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box ./run_tests.sh -d $target_box -f $testbed_file -i $lab_file,$inv_file  -k debug -l info -n $conf_name -t $topology,any -p /output_files/ -r -s ixia crm -y $dev_type -x "
      docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box ./run_tests.sh -d $target_box -f $testbed_file -i $lab_file,$inv_file  -k debug -l info -n $conf_name -t $topology,any -p /output_files/ -r -s "ixia crm" -y $dev_type -x
    elif [[ "$dev_type" == "T2" ]]; then
      inv_files="$lab_file,$inv_file"
      t2_tests_options=""
      if $run_in_order; then
        t2_tests_options="-O "
      fi
      t2_tests_options="$t2_tests_options -T $suite -p /output_files"
      if [ "$ndk_image_url" != "" ]; then
        t2_tests_options="$t2_tests_options -N $ndk_image_url"
      fi
      echo "Executing t2_test.sh with 'docker exec --user=\"$uid:$gid\" -t -w /data/tests sonic_mgmt_$target_box /data/tests/t2_tests.sh $t2_tests_options $conf_name'"
      docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box /data/tests/t2_tests.sh $t2_tests_options $conf_name
    elif [[ "$dev_type" == "T2-vs" ]]; then
       echo "Executing kvmtest.sh with 'docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box /data/tests/kvmtest.sh -T t2 -p /output_files -d /data vms-kvm-t2 vlab-t2-01,vlab-t2-02,vlab-t2-sup"
       docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box /data/tests/kvmtest.sh -T t2 -p /output_files -d /data vms-kvm-t2 vlab-t2-01,vlab-t2-02,vlab-t2-sup
    elif [[ "$topology" == "m0" || "$topology" == "mx" ]]; then
      echo "Executing docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box ./run_tests.sh -a False -d $target_box -i $lab_file,$inv_file -k debug -l info -n $conf_name -t $topology,any -p /output_files/ -r -s ixia  -x  "
      docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box ./run_tests.sh -a False -d $target_box -i $lab_file,$inv_file -k debug -l info -n $conf_name -t $topology,any -p /output_files/ -r -x -u -c "test_pretest.py"
    else
      echo "Executing docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box ./run_tests.sh -a False -d $target_box -i $lab_file,$inv_file -k debug -l info -n $conf_name -t $topology -p /output_files/ -r -s ixia  -x  "
      docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box ./run_tests.sh -a False -d $target_box -i $lab_file,$inv_file -k debug -l info -n $conf_name -t $topology -p /output_files/ -r -s "ixia crm"  -x 
    fi
  else
    upgraded_str=""
    if [[ "$upgrade" == "1" ]]; then
      upgraded_str=" --is_upgraded"
    fi
    inv_files="$lab_file,$inv_file"
    echo "Executing express_test.sh with 'docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box /data/tests/express_tests.sh -p /output_files $conf_name"
    docker exec --user="$uid:$gid" -t -w /data/tests sonic_mgmt_$target_box /data/tests/express_tests.sh -p /output_files $conf_name
  fi
  if [[ "$?" == "0" ]]; then
      test_failed=0
  else
      test_failed=1
  fi

  # post and analyze the results if required
  if [[ "$post_results" == "1" ]]; then
    # analyze the results using scripts in altos/tests to generate openissues.json
    analyze_results $pytest_results_dir
  fi

  if [[ "$test_failed" == "0" ]]; then
    echo "tests for topology '$topology' of '$image_url' on '$target_box' passed"
    postrun_regress_db $notify $test_failed $pytest_results_dir $basedir $testbed_server_name
  else
    echo "tests for topology '$topology' of '$image_url' on '$target_box' did not pass, tar file dumps for the failed tests from failures can be found in '$dump_dir'"
    postrun_regress_db $notify $test_failed $pytest_results_dir $basedir $testbed_server_name
    run_test_exit 1
  fi
fi

run_test_exit 0


