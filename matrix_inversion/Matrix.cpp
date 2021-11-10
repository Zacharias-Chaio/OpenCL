#include "Matrix.hpp"
#include <sstream>

using namespace std;


// Permuter deux rangées de la matrice.
Matrix &Matrix::swapRows(size_t iR1, size_t iR2) {
    // vérifier la validité des indices de rangée
    assert (iR1 < rows() && iR2 < rows());
    // tester la nécessité de permuter
    if (iR1 == iR2) return *this;
    // permuter les deux rangées
    valarray<double> lTmp(mData[slice(iR1 * cols(), cols(), 1)]);
    mData[slice(iR1 * cols(), cols(), 1)] = mData[slice(iR2 * cols(), cols(), 1)];
    mData[slice(iR2 * cols(), cols(), 1)] = lTmp;
    return *this;
}

// Permuter deux colonnes de la matrice.
Matrix &Matrix::swapColumns(size_t iC1, size_t iC2) {
    // vérifier la validité des indices de colonne
    assert (iC1 < cols() && iC2 < cols());
    // tester la nécessité de permuter
    if (iC1 == iC2) return *this;
    // permuter les deux rangées
    valarray<double> lTmp(mData[slice(iC1, rows(), cols())]);
    mData[slice(iC1, rows(), cols())] = mData[slice(iC2, rows(), cols())];
    mData[slice(iC2, rows(), cols())] = lTmp;
    return *this;
}

// Représenter la matrice sous la forme d'une chaîne de caractères.
// Pratique pour le débuggage...
string Matrix::str(void) const {
    ostringstream oss;
    for (size_t i = 0; i < rows(); ++i) {
        if (i == 0) oss << "[[ ";
        else oss << " [ ";
        for (size_t j = 0; j < cols(); ++j) {
            oss << (*this)(i, j);
            if (j + 1 != cols()) oss << ", ";
        }
        if (i + 1 == this->rows()) oss << "]]";
        else oss << " ]," << endl;
    }
    return oss.str();
}

// Construire une matrice identité.
MatrixIdentity::MatrixIdentity(size_t iSize) : Matrix(iSize, iSize) {
    for (size_t i = 0; i < iSize; ++i) {
        (*this)(i, i) = 1.0;
    }
}

// Construire une matrice aléatoire [0,1) iRows x iCols.
// Utiliser srand pour initialiser le générateur de nombres.
MatrixRandom::MatrixRandom(size_t iRows, size_t iCols) : Matrix(iRows, iCols) {
    for (size_t i = 0; i < mData.size(); ++i) {
        mData[i] = (double) rand() / RAND_MAX;;
    }
}

MatrixExample::MatrixExample(size_t iRows, size_t iCols) : Matrix(iRows, iCols) {
    for (size_t i = 0; i < mData.size(); ++i) {
        mData[i] = (double) ((i + 2.0) * 2.0);
    }
}


// Construire une matrice en concaténant les colonnes de deux matrices de même hauteur.
MatrixConcatCols::MatrixConcatCols(const Matrix &iMat1, const Matrix &iMat2) : Matrix(iMat1.rows(),
                                                                                      iMat1.cols() + iMat2.cols()) {
    // vérifier la compatibilité des matrices
    assert (iMat1.rows() == iMat2.rows());
    // Pour chaque rangée
    for (size_t i = 0; i < rows(); ++i) {
        // rangée i de la première matrice
        mData[slice(i * cols(), iMat1.cols(), 1)] = iMat1.getRowSlice(i);
        // rangée i de la seconde matrice
        mData[slice(i * cols() + iMat1.cols(), iMat2.cols(), 1)] = iMat2.getRowSlice(i);
    }
}

// Construire une matrice en concaténant les rangées de deux matrices de même largeur.
MatrixConcatRows::MatrixConcatRows(const Matrix &iMat1, const Matrix &iMat2) : Matrix(iMat1.rows() + iMat2.rows(),
                                                                                      iMat1.cols()) {
    // vérifier la compatibilité des matrices
    assert (iMat1.cols() == iMat2.cols());
    // Pour chaque colonne
    for (size_t j = 0; j < cols(); ++j) {
        // colonne j de la première matrice
        mData[slice(j, iMat1.rows(), cols())] = iMat1.getColumnSlice(j);
        // colonne j de la seconde matrice
        mData[slice(j + iMat1.rows(), iMat2.rows(), cols())] = iMat2.getColumnSlice(j);
    }
}

// Insérer une matrice dans un flot de sortie.
ostream &operator<<(ostream &oStream, const Matrix &iMat) {
    oStream << iMat.str();
    return oStream;
}
