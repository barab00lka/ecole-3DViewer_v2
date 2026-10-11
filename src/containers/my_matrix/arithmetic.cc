#include <stdexcept>

#include "my_matrix_oop.h"

using namespace ec;
bool Matrix::EqMatrix(const Matrix& o) const {
  if (IsEqSize(o) == false) {
    return false;
  }
  for (auto i = 0; i < rows_; ++i) {
    for (auto j = 0; j < cols_; ++j) {
      if (std::abs(o(i, j) - (*this)(i, j)) > kEpsilon) {
        return false;
      }
    }
  }
  return true;
}

void Matrix::SumMatrix(const Matrix& o) {
  if (IsEqSize(o) == false) {
    throw std::out_of_range(
        "Incorrect input, matrices should have the same size");
  }
  for (auto i = 0; i < rows_; ++i) {
    for (auto j = 0; j < cols_; ++j) {
      (*this)(i, j) += o(i, j);
    }
  }
}

void Matrix::SubMatrix(const Matrix& o) {
  if (IsEqSize(o) == false) {
    throw std::out_of_range(
        "Incorrect input, matrices should have the same size");
  }
  for (auto i = 0; i < rows_; ++i) {
    for (auto j = 0; j < cols_; ++j) {
      (*this)(i, j) -= o(i, j);
    }
  }
}

void Matrix::MulNumber(const float num) noexcept {
  for (auto i = 0; i < rows_; ++i) {
    for (auto j = 0; j < cols_; ++j) {
      (*this)(i, j) *= num;
    }
  }
}

void Matrix::MulMatrix(const Matrix& other) {
  if (IsMul(other) == false) {
    throw std::out_of_range(
        "The number of columns of the first matrix is not equal to the number "
        "of rows of the second matrix.");
  }
  Matrix res(this->rows_, other.cols_);
  for (auto n = 0; n < rows_; ++n) {
    for (auto i = 0; i < cols_; ++i) {
      for (auto j = 0; j < other.cols_; ++j) {
        res(n, j) += (*this)(n, i) * other(i, j);
      }
    }
  }

  *this = res;
}

Matrix Matrix::Transposed() const {
  Matrix n(cols_, rows_);
  for (auto i = 0; i < rows_; ++i) {
    for (auto j = 0; j < cols_; ++j) {
      n(j, i) = (*this)(i, j);
      ;
    }
  }
  return n;
}

Matrix Matrix::Minor(int sRow, int sCol) const {
  Matrix m(rows_ - 1, cols_ - 1);
  for (auto i = 0, mi = 0; i < rows_; ++i) {
    if (i != sRow) {  // пропуск строки в миноре
      for (auto j = 0, mj = 0; j < cols_; ++j)
        if (j != sCol) {  // пропуск столбца в миноре
          m(mi, mj) = (*this)(i, j);
          mj += 1;
        }
      mi += 1;
    }
  }
  return m;
}

// Using Gauss method O(n^3) with Partial pivoting.
float Matrix::Determinant() const {
  if (!IsSquare())
    throw std::invalid_argument(
        "Cannot calculate minor matrix: parent matrix must be square.");

  if (rows_ == 0)
    return 1.0;
  else if (rows_ == 1)
    return (*this)(0, 0);
  else if (rows_ == 2)
    return (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);

  Matrix temp(*this);
  float det = 1.0;
  int n = rows_;

  for (int i = 0; i < n; ++i) {
    // Partial pivoting
    int pivot_row = i;
    for (int j = i + 1; j < n; ++j)
      if (std::abs(temp(j, i)) > std::abs(temp(pivot_row, i))) pivot_row = j;

    if (std::abs(temp(pivot_row, i)) < kEpsilon) {
      det = 0.0;
      break;
    }

    temp.SwapRows(i, pivot_row);

    det *= temp(i, i);

    if (pivot_row != i) det = -det;

    for (int j = i + 1; j < n; ++j) {
      float koef = temp(j, i) / temp(i, i);
      for (int k = i; k < n; ++k) temp(j, k) -= temp(i, k) * koef;
    }
  }
  return det;
}

Matrix Matrix::CalcComplements() const {
  if (IsSquare() == false) {
    throw std::logic_error(
        "Cannot calculate components: matrix must be square.");
  }
  Matrix result{rows_, cols_};

  if (rows_ == 1) {
    result(0, 0) = 1.0;
    return result;
  }

  for (int i = 0; i < result.get_rows(); ++i) {
    for (int j = 0; j < result.get_cols(); ++j) {
      Matrix minor_matrix = Minor(i, j);
      result(i, j) = minor_matrix.Determinant();
      if ((i + j) & 1) {
        result(i, j) = -result(i, j);
      }
    }
  }
  return result;
}

Matrix Matrix::InverseMatrix() const {
  if (IsSquare() == false) {
    throw std::logic_error("Incorrect matrix size for Inverse");
  }

  float det = Determinant();

  if (std::abs(det) < kEpsilon) {
    throw std::logic_error("Determinant must be non-zero to calculate Inverse");
  }

  return Transposed().CalcComplements() * (1.0 / det);
}
