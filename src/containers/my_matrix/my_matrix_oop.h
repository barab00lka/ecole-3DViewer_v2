#ifndef __MYMATRIX_H__
#define __MYMATRIX_H__

#include <memory>
namespace ec {

class Matrix {
 public:
  Matrix() noexcept;             // default constructor
  Matrix(int rows, int cols);    // parameterized constructor
  Matrix(const Matrix& o);       // copy constructor
  Matrix(Matrix&& o) noexcept;   // move constructor
  ~Matrix() noexcept = default;  // destructor

  // public getters and setters
  int get_cols() const noexcept;
  int get_rows() const noexcept;
  void set_rows(int new_rows);  // resizes underlying matrix_ automatically
  void set_cols(int new_cols);  // resizes underlying matrix_ automatically

  // operators overloads
  Matrix& operator=(Matrix o) noexcept;  // move-by-value assignment operator
  // safe indexing with bounds checking. can be turned off
  // if compiled with NDEBUG
  float& operator()(int row, int col) &;
  const float& operator()(int row, int col) const&;

  // arithmetic operators
  bool operator==(const Matrix& o) const;
  Matrix& operator+=(const Matrix& o);
  Matrix operator+(const Matrix& o) const;
  Matrix& operator-=(const Matrix& o);
  Matrix operator-(const Matrix& o) const;
  Matrix& operator*=(const float number);
  Matrix& operator*=(const Matrix& o);
  Matrix operator*(const Matrix& o) const;
  Matrix operator*(const float number) const noexcept;

  // overloading of float * matrix (float is lvalue). 'Friend' is needed here
  // because lvalue is not a class object thus not being treated as a class
  // method. Friend allows to bind it to this class.
  friend Matrix operator*(float number, const Matrix& matrix) noexcept;

  // arithmetic methods
  bool EqMatrix(const Matrix& o) const;
  void SumMatrix(const Matrix& o);
  void SubMatrix(const Matrix& o);
  void MulNumber(const float num) noexcept;
  void MulMatrix(const Matrix& other);
  Matrix Transposed() const;
  Matrix Minor(int sRow, int sCol) const;
  Matrix CalcComplements() const;
  float Determinant() const;
  Matrix InverseMatrix() const;

 protected:
  float* data() noexcept { return matrix_.get(); }
  const float* data() const noexcept { return matrix_.get(); }

 private:
  static constexpr float kEpsilon = 1e-7;

  bool IsEqSize(const Matrix& o) const noexcept {
    return (cols_ == o.cols_ && rows_ == o.rows_);
  }
  bool IsMul(const Matrix& o) const noexcept { return (cols_ == o.rows_); }
  bool IsSquare() const noexcept { return rows_ == cols_; }
  void SwapRows(int row1, int row2) {
    if (row1 != row2) {
      for (int i = 0; i < cols_; ++i) {
        std::swap((*this)(row1, i), (*this)(row2, i));
      }
    }
  }

 private:
  int rows_, cols_;
  std::unique_ptr<float[]> matrix_;
};
}  // namespace ec

#endif
