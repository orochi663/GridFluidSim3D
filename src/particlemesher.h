/*
Copyright (c) 2015 Ryan L. Guy

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#ifndef PARTICLEMESHER_H
#define PARTICLEMESHER_H

#include <stdio.h>
#include <iostream>

#include "array3d.h"
#include "grid3d.h"
#include "spatialpointgrid.h"
#include "aabb.h"
#include "levelset.h"
#include "trianglemesh.h"
#include "implicitsurfacescalarfield.h"
#include "polygonizer3d.h"
#include "stopwatch.h"
#include "threading.h"
#include "vmath.h"
#include "fluidmaterialgrid.h"
#include "fragmentedvector.h"
#include "markerparticle.h"

class ParticleMesher
{
public:
    ParticleMesher();
    ParticleMesher(int isize, int jsize, int ksize, double dx);
    ~ParticleMesher();

    TriangleMesh meshParticles(FragmentedVector<MarkerParticle> &particles, 
                               LevelSet &levelset,
                               FluidMaterialGrid &materialGrid,
                               double particleRadius);

    void _runSmoothRangeOfSurfaceParticlePositionsThread(int startidx, int endidx);
    void _runComputeRangeOfCovarianceMatricesThread(int startidx, int endidx);
private: 

    struct SurfaceParticle {
        vmath::vec3 position;
        GridPointReference ref;
        int componentID;

        SurfaceParticle() : ref(-1),
                            componentID(-1) {}
        SurfaceParticle(vmath::vec3 p) : position(p), 
                                         ref(-1),
                                         componentID(-1) {}
        SurfaceParticle(vmath::vec3 p, GridPointReference r) : 
                                         position(p), 
                                         ref(r),
                                         componentID(-1) {}
        SurfaceParticle(vmath::vec3 p, GridPointReference r, int compID) : 
                                         position(p), 
                                         ref(r),
                                         componentID(compID) {}
    };

    struct IsotropicParticle {
        vmath::vec3 position;

        IsotropicParticle() {}
        IsotropicParticle(vmath::vec3 p) : position(p) {}
    };

    struct AnisotropicParticle {
        vmath::vec3 position;
        vmath::mat3 anisotropy;

        AnisotropicParticle() {}
        AnisotropicParticle(vmath::vec3 p) : position(p), 
                                           anisotropy(vmath::mat3(1.0)) {}
        AnisotropicParticle(vmath::vec3 p, vmath::mat3 G) : position(p), 
                                                        anisotropy(G) {}
    };

    struct SVD {
        vmath::mat3 rotation;
        vmath::vec3 diag;

        SVD() {}
        SVD(vmath::mat3 rot, vmath::vec3 d) : rotation(rot), diag(d) {}
        SVD(vmath::vec3 d, vmath::mat3 rot) : rotation(rot), diag(d) {}
    };

    void _clear();
    void _computeSurfaceReconstructionParticles(FragmentedVector<MarkerParticle> &particles, 
                                                LevelSet &levelset,
                                                FragmentedVector<IsotropicParticle> &iso,
                                                FragmentedVector<AnisotropicParticle> &aniso);
    void _filterHighDensityParticles(FragmentedVector<MarkerParticle> &particles, 
                                     FragmentedVector<vmath::vec3> &filtered);
    void _sortParticlesBySurfaceDistance(FragmentedVector<vmath::vec3> &allParticles,
                                         FragmentedVector<vmath::vec3> &insideParticles,
                                         FragmentedVector<vmath::vec3> &surfaceParticles,
                                         FragmentedVector<int> &nearSurfaceParticles,
                                         FragmentedVector<int> &farSurfaceParticles,
                                         LevelSet &levelset);
    void _initializeSurfaceParticleSpatialGrid(FragmentedVector<vmath::vec3> &particles);
    void _updateNearFarSurfaceParticleReferences(FragmentedVector<int> nearParticles,
                                                 FragmentedVector<int> farParticles);
    void _updateSurfaceParticleComponentIDs();
    void _smoothSurfaceParticlePositions();
    void _computeSmoothedNearSurfaceParticlePositions();
    void _smoothRangeOfSurfaceParticlePositions(int startidx, int endidx);
    vmath::vec3 _getSmoothedParticlePosition(GridPointReference ref,
                                           double radius,
                                           std::vector<GridPointReference> &refs);
    vmath::vec3 _getWeightedMeanParticlePosition(GridPointReference ref,
                                               std::vector<GridPointReference> &neighbours);

    void _computeAnisotropyMatrices(FragmentedVector<vmath::mat3> &matrices);
    void _computeCovarianceMatrices();
    void _computeRangeOfCovarianceMatrices(int startidx, int endidx);
    vmath::mat3 _computeCovarianceMatrix(GridPointReference ref, double radius,
                                       std::vector<GridPointReference> &neighbours);
    void _computeSVDMatrices();
    void _covarianceMatrixToSVD(vmath::mat3 &covariance, SVD &svd);
    vmath::quat _diagonalizeMatrix(vmath::mat3 A);
    vmath::mat3 _SVDToAnisotropicMatrix(SVD &svd);

    void _initializeSurfaceReconstructionParticles(FragmentedVector<IsotropicParticle> &iso,
                                                   FragmentedVector<vmath::vec3> &insideParticles,
                                                   FragmentedVector<AnisotropicParticle> &aniso,
                                                   FragmentedVector<vmath::mat3> &anisoMatrices);

    TriangleMesh _reconstructSurface(FragmentedVector<IsotropicParticle> &iso,
                                     FragmentedVector<AnisotropicParticle> &aniso,
                                     FluidMaterialGrid &materialGrid);

    void _setParticleRadius(double r);
    void _setKernelRadius(double r);
    double _evaluateKernel(SurfaceParticle &pi, SurfaceParticle &pj);

    double _particleRadius = 0.0;
    double _anisotropicParticleScale = 0.7;
    double _anisotropicParticleFieldScale = 0.4;
    double _isotropicParticleScale = 3.0;
    double _kernelRadius = 0.0;
    double _invKernelRadius = 1.0;

    double _supportRadiusFactor = 3.0;              // in number of _particleRadius
    double _connectedComponentRadiusFactor = 2.5;   // in number of _particleRadius
    double _smoothingConstant = 0.95;               // in range [0.0,1.0]
    int _numThreads = 8;

    int _minAnisotropicParticleNeighbourThreshold = 10;
    double _maxEigenvalueRatio = 5.5;

    int _isize = 0;
    int _jsize = 0;
    int _ksize = 0;
    double _dx = 0;

    int _maxParticlesPerCell = 16;
    double _maxParticleToSurfaceDepth = 1.0; // in number of cells

    SpatialPointGrid _pointGrid;
    FragmentedVector<SurfaceParticle> _surfaceParticles;
    FragmentedVector<GridPointReference> _nearSurfaceParticleRefs;
    FragmentedVector<GridPointReference> _farSurfaceParticleRefs;
    FragmentedVector<vmath::vec3> _smoothedPositions;
    FragmentedVector<vmath::mat3> _covarianceMatrices;
    FragmentedVector<SVD> _SVDMatrices;
};

#endif
