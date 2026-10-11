#include "my_matrix_oop.h"
using namespace ec;

int Matrix::get_rows() const noexcept { return rows_; }

int Matrix::get_cols() const noexcept { return cols_; }

void Matrix::set_rows(int new_rows) {
  if (new_rows < 0) {
    throw std::length_error("matrix rows count must be non-negative");
  }

  if (new_rows != rows_) {
    // Работа с копией позволяет нам соблюдать строгую гарантию безопасности
    // исключений
    Matrix tmp{new_rows, cols_};
    int min = std::min(rows_, new_rows);
    for (int i = 0; i < min; ++i) {
      for (int j = 0; j < cols_; ++j) {
        tmp(i, j) = (*this)(i, j);
      }
    }
    // Копировать дорого, а матрица tmp нам больше не нужна, поэтому используем
    // перемещение вместо копирования (что бесплатно)
    *this = std::move(tmp);
  }
}

void Matrix::set_cols(int new_cols) {
  if (new_cols < 0) {
    throw std::length_error("matrix columns count must be non-negative");
  }

  if (new_cols != cols_) {
    Matrix tmp{rows_, new_cols};
    int min = std::min(cols_, new_cols);
    for (int i = 0; i < rows_; ++i) {
      for (int j = 0; j < min; ++j) {
        tmp(i, j) = (*this)(i, j);
      }
    }
    *this = std::move(tmp);
  }
}
