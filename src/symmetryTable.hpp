#ifndef __SYMMETRYTABLE_H__
#define __SYMMETRYTABLE_H__

#include <vector>

#include <maya/MDagPath.h>
#include <maya/MPxCommand.h>

struct Face {
    bool isDone;
    int index;
    int opposite = -1;
    std::vector<int> edges;
    std::vector<int> vertices;
};

struct Edge {
    int faces[2];
    int vertices[2];
    int opposite = -1;
    bool onBoundary;
};

struct Task {
    int faceAIndex;
    int faceBIndex;
    int edgeAIndex;
    int edgeBIndex;
};

class SymmetryTable : public MPxCommand {
public:
    SymmetryTable();
    virtual ~SymmetryTable();
    MStatus doIt(const MArgList& argList);
    MStatus undoIt();
    MStatus redoIt();
    bool isUndoable() const;
    static void* creator();
    static MSyntax newSyntax();

    MStatus initMesh(MDagPath& dagPath);

private:
    std::vector<int> vertices;
    std::vector<Edge> edges;
    std::vector<Face> faces;

    bool outVertex;
    bool outEdge;
    bool outFace;
    bool verbose;
    bool halfOnly;
};

inline int findIndex(std::vector<int>& v, int k)
{
    int inc = 0;
    for (int i : v) {
        if (i == k) {
            return inc;
        }
        inc++;
    }
    return -1;
}

#endif /* defined(__SYMMETRYTABLE_H__) */