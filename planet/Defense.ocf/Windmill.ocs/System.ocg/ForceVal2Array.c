// helper: put single elements into array

global func ForceVal2Array(v) {  if (GetType(v) != C4V_Array) return [v]; else return v; }