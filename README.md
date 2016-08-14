# sequential-knowledge-based-IR

## Paper Title
Sequential Query Expansion using Concept Graph

## Paper Abstract
Manually and automatically constructed concept graphs (or semantic networks), in which the nodes correspond to terms and phrases with typed edges designating the relationships between them, have been previously shown to be rich sources of effective latent concepts for query expansion. However, finding good expansion concepts for a given query in large and dense concept graph is a challenging problem, since the number of related concepts that need to be examined generally increases exponentially with the distance to the original query concepts, which leads to topic drift. In this paper, we propose a feature-based method to sequentially select the most effective concepts for query expansion from a concept graph. The proposed method first weighs the concepts according to different types of features, including collection and concept graph statistics. After that, a feature-based sequential concept selection algorithm is applied to find the most effective expansion concepts at different distances from query concepts. Experiments on different types of TREC datasets indicate a significant improvement in retrieval accuracy that can be achieved by utilizing the proposed method for query expansion using concept graphs over state-of-the-art methods.

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
