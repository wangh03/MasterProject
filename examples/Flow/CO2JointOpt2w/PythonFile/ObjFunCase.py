import sys
from ObjFunClass import ObjFunCla,NPVCla

optimal_model_dir = sys.argv[1]
py_result_dir = sys.argv[2]
unsmary_dir = '{}/CO2OPT2W'.format(optimal_model_dir)

NPVcomponent = ({'datatype': 'Field', 'wellname': 'INJ1', 'fluidtype': 'Gas', 'flowtype': 'Injection', \
       'interval': 'yearly', 'fluidprice': 15, 'discountfactor': 0.08},
      {'datatype': 'Field', 'wellname': 'PROD1', 'fluidtype': 'Oil', 'flowtype': 'Production', \
       'interval': 'yearly', 'fluidprice': -10, 'discountfactor': 0.08}, \
      {'datatype': 'Field', 'wellname': 'PROD1', 'fluidtype': 'Gas', 'flowtype': 'Production', \
       'interval': 'yearly', 'fluidprice': -500, 'discountfactor': 0.08})


thipar = foupar = fifpar = 0
NPVcase = NPVCla(unsmary_dir, NPVcomponent, thipar, foupar, fifpar)
Objcase = ObjFunCla(unsmary_dir, NPVcomponent, thipar, foupar, fifpar)

# call the NPV calculation property.
NPVcase.NPVsumcal
Objcase.objfunvalcal()

objective_result = Objcase.objfunvalcal()

# write out the objective function result to a txt file
py_result_path = '{}/PythonObjeResult.txt'.format(py_result_dir)
with open(py_result_path, 'w') as file:
     file.write(objective_result)
