#include <cassert>
#include <cstring>
#include <memory>
#include <stdexcept>

#include "my_matrix_oop.h"
using namespace ec;

Matrix::Matrix() noexcept : rows_(0), cols_(0), matrix_(nullptr) {}

Matrix::Matrix(int rows, int cols) : rows_(rows), cols_(cols) {
  if (rows_ < 0 || cols_ < 0) {
    throw std::length_error("Matrix size must be greater or equal than 0");
  }
  matrix_ = std::make_unique<float[]>(
      rows_ * cols_);  // () does zero value initialization
}

// Copy constructor
Matrix::Matrix(const Matrix& o)
    : rows_(o.rows_),
      cols_(o.cols_),
      matrix_(o.matrix_ ? std::make_unique_for_overwrite<float[]>(rows_ * cols_)
                        : nullptr) {
  std::memcpy(matrix_.get(), o.matrix_.get(), rows_ * cols_ * sizeof(float));
}

// Move constructor
Matrix::Matrix(Matrix&& o) noexcept
    : rows_(o.rows_), cols_(o.cols_), matrix_(std::move(o.matrix_)) {
  o.rows_ = 0, o.cols_ = 0;
}
