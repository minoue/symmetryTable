# symmetryTable

Maya plugin to find corresponding vertices, edges, or faces based on topological symmetry table.

**This is a plugin that only creates symmetry tables and does not include any tools such as mirroring vertex positions/weights.**

## Build
for Maya2022 linux/macOS

```
> mkdir build
> cd build
> cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DMAYA_ROOT_DIR="path/to/maya/dir" -DMAYA_VERSION=2022 ../
> cmake --build . --config Release --target install
```

Windows
```
>
```

## How to use
Select an edge where is a middle of the topology, then run the command

<img src="./img/middleEdge.png" width="350">


## Flags
| Longname | Shortname | Argument types | Default | Properties |
|:---------|----------:|:--------------:|:-------:|:----------:|
|vertex|v|bool|true|C|
|edge|e|bool|false|C|
|face|f|bool|false|C|
|verbose|vb|bool|false|C|

## Example
```python
from maya import cmds
vtxs = cmds.createSymmetryTable()
print(vtxs)
> [8L, 7L, 6L, 5L, 4L, 3L, 2L, 1L, 0L, 19L, 18L, 17L, 16L, ... ]
 #, where the corresponding vertex of the first vertex is 8, the second vertex is 7, the third is 6, and so on...

```
<img src="./img/vtxPairs.png" width="350">