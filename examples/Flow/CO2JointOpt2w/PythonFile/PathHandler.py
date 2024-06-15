# ------------------------------------------------------------
# This file is used for the assignment of the result path to json
# ------------------------------------------------------------

import sys
import json

json_drive_path = sys.argv[1]
python_result_dir = sys.argv[2]
python_result_path = "{}/PythonObjeResult.txt".format(python_result_dir)

with open(json_drive_path, 'r') as file:
    data = json.load(file)

    data["Optimizer"]["Objective"]["ExternalResultComponent"]["ExternalFilePath"] = python_result_path
    
    with open(json_drive_path, 'w') as file:
        json.dump(data, file, indent=4)

with open(python_result_path, 'w') as file:
    pass
