#include <cassert>

#include "my_matrix_oop.h"
using namespace ec;

// index (AT) operator
float& Matrix::operator()(int row, int col) & {
#ifndef NDEBUG
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_)
    throw std::out_of_range("Index out of range");
#endif
  return matrix_[(row * cols_) + col];
}

// index (AT) operator const read-only reference
const float& Matrix::operator()(int row, int col) const& {
#ifndef NDEBUG
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_)
    throw std::out_of_range("Index out of range");
#endif
  return matrix_[(row * cols_) + col];
}

Matrix& Matrix::operator=(Matrix o) noexcept {
  std::swap(rows_, o.rows_);
  std::swap(cols_, o.cols_);
  std::swap(matrix_, o.matrix_);
  return *this;
}

Matrix Matrix::operator+(const Matrix& o) const {
  Matrix res(*this);
  return res += o;
}

Matrix& Matrix::operator+=(const Matrix& o) {
  SumMatrix(o);
  return *this;
}

Matrix Matrix::operator-(const Matrix& o) const {
  Matrix res(*this);
  return res -= o;
}

Matrix& Matrix::operator-=(const Matrix& o) {
  SubMatrix(o);
  return *this;
}

Matrix operator*(float number, const Matrix& matrix) noexcept {
  Matrix tmp = matrix * number;
  return tmp;
}

Matrix Matrix::operator*(const Matrix& o) const {
  Matrix res(*this);
  res.MulMatrix(o);
  return res;
}

Matrix& Matrix::operator*=(const Matrix& o) {
  MulMatrix(o);
  return *this;
}

Matrix Matrix::operator*(const float num) const noexcept {
  Matrix res(*this);
  res.MulNumber(num);
  return res;
}

Matrix& Matrix::operator*=(const float num) {
  MulNumber(num);
  return *this;
}

bool Matrix::operator==(const Matrix& o) const { return EqMatrix(o); }
