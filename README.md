# sequential-knowledge-based-IR

## estimating model parameters by using Coordinate Ascent method

- Nm = min # of concepts from the previous layer that are related to the selectable concepts
- STY = semantic type

### using infNDCG as the objective function

| STY | Nm            | intCoeff |fbD, fbT| infNDCG |
| --- | ------------- | -------- | ------ | ------- |
|  Y  | 0             |  0.01    | 0      |0.215    |
|  Y  | 1\*           |  0.01    | 0      |0.2163   |
|  Y  | 100 (inf)     |  0.01    | 0      |0.2163   |
|  Y  | 1             |  0.05\*  | 0      |0.2187   |
|  Y  | 1             |  0.1     | 0      |0.217    |
|  Y  | 1             |  0.025   | 0      |0.2169   |
|  Y  | 2             |  0.05    | 0      |0.2185   |
|  Y  | 3             |  0.05    | 0      |0.2169   |
|  Y  | 100 (inf)     |  0.05    | 0      |0.2163   |
|  Y  | 1             |  0.05    | 30, 28 |0.2872   |
|  Y  | 1             |  0.01    | 30, 28 |0.2889   |

### using MAP as the objective function

| STY | Nm            | intCoeffs       |fbD, fbT| MAP     |
| --- | ------------- | --------------- | ------ | ------- |
|  Y  | 100 (inf)     |  0.95, 0.0, 0.0 | 30, 28 |         |
