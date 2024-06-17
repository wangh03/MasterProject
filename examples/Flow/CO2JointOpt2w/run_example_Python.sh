#!/bin/bash
# This is a bash script for running the example with external
# NPV calculations. This example uses two different 
# optimization algorithms.

# Check if the user provided two parameters.
# -----------------------------------------------------------
if [ $# -ne 2 ]; then
    echo "Usage: $0 execution help "
    echo "agr1:<ALGO_ABBR> the algorithm abbreviation to run. GA/PSO"
    echo "agr2:<PYTHON_DIR> the python directory includes .py"
    exit 1 # indicates a failed run
fi

# Store the parameters provided by the user.
# ------------------------------------------------------------
ALGO_ABBR="$1" # the optimization algorithm: GA or PSO.
REA_PY_DIR="$2" # the relative Python script path that will be executed for the external objective function calculation.

# ---------------------------------------------------------
# store the path variables for the simulator/FieldOpt/python interpreter.

FIELDOPT_BIN=FieldOpt # this line gives the directory for FieldOpt.
CURRENT_DIR=$(pwd) # get the present working directory.
FIELDOPT_OUT=${CURRENT_DIR}/Output # the directory to store the optimization results.
MODEL_DIR=${CURRENT_DIR}/CO2OPT2W # the directory of the base case model.
DECK_PATH=${MODEL_DIR}/CO2OPT2W.DATA # .DATA file path of the base case model.
GRID_PATH=${MODEL_DIR}/CO2OPT2W.EGRID # .EGRID file path of the base case model.
OUTPUT_DIR=${FIELDOPT_OUT}/${ALGO_ABBR}
UNSMARY_DIR=${OUTPUT_DIR}/CO2OPT2W
ABS_PY_DIR=${CURRENT_DIR}/${REA_PY_DIR} # abosulute python script directory.
JSON_DRIVER_FILE=${CURRENT_DIR}/fo_driver.CO2OPT.${ALGO_ABBR}.json

#-----------------------------------------------------------
# simulate the base case model
/usr/bin/flow "${DECK_PATH}" >/dev/null

# form a string appending to bash_flw_bin.sh
py_exec_str="/usr/bin/python3 "${ABS_PY_DIR}"ObjFunCase.py \"${UNSMARY_DIR}\" \"${OUTPUT_DIR}\""

rm -f bash_flw_py_"${ALGO_ABBR}".sh

# append the Python execution string to the bash file for the FieldOpt input
sed -e '$a\'$'\n'"${py_exec_str}" base_bash.sh > bash_flw_py_"${ALGO_ABBR}".sh

chmod +x bash_flw_py_"${ALGO_ABBR}".sh

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

/usr/bin/python3 "${ABS_PY_DIR}"PathHandler.py "${JSON_DRIVER_FILE}" "${OUTPUT_DIR}"

# Make run command (SERIAL)
CMDSER="${FIELDOPT_BIN} "\
"${JSON_DRIVER_FILE} ${OUTPUT_DIR} -r serial "\
"-g ${GRID_PATH} -s ${DECK_PATH} "\
"-e bash_flw_py_"${ALGO_ABBR}".sh -f -v 3 -t 0"

# printf '%s\n' "CMDSER=${CMDSER}"
eval ${CMDSER}
