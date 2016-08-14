/*
 * Template for Sparse Matrix
 * Author: Alexander Kotov, UIUC, 2010
 *
 */

#ifndef SPARSE_MATRIX_HPP_
#define SPARSE_MATRIX_HPP_

#include "Exception.hpp"
#include <assert.h>
#include <ostream>
#include <map>
#include <cstdio>

template<class T> class SparseMatrix;

template<class T> class Row {
public:
  typedef typename std::map<unsigned int, T> ColMap;
  typedef typename ColMap::iterator ColMapIt;
  typedef typename ColMap::const_iterator cColMapIt;
  typedef typename ColMap::reverse_iterator rColMapIt;
  typedef typename ColMap::const_reverse_iterator crColMapIt;

public:
  Row() : _def(0)
  { }

  Row(T def) : _def(def)
  { }

  T& operator[](unsigned int c) {
    ColMapIt it;
    if((it = _cols.find(c)) == _cols.end()) {
      return _def;
    } else {
      return it->second;
    }
  }

  bool get(unsigned int c, T* val) {
    ColMapIt it;
    if((it = _cols.find(c)) != _cols.end()) {
      if(val != NULL) {
        *val = &it->second;
      }
      return true;
    } else {
      if(val != NULL) {
        val = NULL;
      }
      return false;
    }
  }

  void set(unsigned int c, const T& val) {
    _cols[c] = val;
  }

  inline size_t numCols() const {
    return _cols.size();
  }

  size_t maxColIndex() const {
    return _cols.size() ? _cols.rbegin()->first : 0;
  }

  friend class SparseMatrix<T>;
  friend class SparseMatrix<T>::ColIterator;
  friend class SparseMatrix<T>::ConstColIterator;

protected:
  ColMap _cols;
  T _def;
};

template<class T> class SparseMatrix {
protected:
  typedef typename std::map<unsigned int, Row<T> > RowMap;
  typedef typename RowMap::iterator RowMapIt;
  typedef typename RowMap::reverse_iterator rRowMapIt;
  typedef typename RowMap::const_iterator cRowMapIt;
  typedef typename RowMap::const_reverse_iterator crRowMapIt;

public:
  SparseMatrix() : _def(0)
  { }

  SparseMatrix(T def) : _def(def)
  { }

  SparseMatrix(T **mat, size_t size) {
    size_t i, j;
    for(i = 0; i < size; i++) {
      for(j = 0; j < size; j++) {
        set(i, j, mat[i][j]);
      }
    }
  }

  virtual ~SparseMatrix()
  { }

  Row<T>& operator[](const unsigned int r) {
    RowMapIt it;
    if((it = _rows.find(r)) == _rows.end()) {
      Row<T> row(_def);
      return row;
    } else {
      return it->second;
    }
  }

  void set(unsigned int r, unsigned int c, const T& val) {
    RowMapIt it;
    if((it = _rows.find(r)) == _rows.end()) {
      Row<T> row(_def);
      row._cols[c] = val;
      _rows[r] = row;
    } else {
      it->second._cols[c] = val;
    }
  }

  Row<T>& addRow(const unsigned int r) {
    RowMapIt it;
    if((it = _rows.find(r)) == _rows.end()) {
      Row<T> row(_def);
      _rows[r] = row;
      return _rows[r];
    } else {
      return it->second;
    }
  }

  Row<T>& get(unsigned int r) throw(lemur::api::Exception) {
    RowMapIt rit;

    if((rit = _rows.find(r)) != _rows.end()) {
      return rit->second;
    } else {
      char msg[128];
      snprintf(msg, 128, "row %d is not found", r);
      throw lemur::api::Exception("SparseMatrix::get(r)", msg);
    }
  }

  const Row<T>& get(unsigned int r) const throw(lemur::api::Exception) {
    cRowMapIt rit;

    if((rit = _rows.find(r)) != _rows.end()) {
      return rit->second;
    } else {
      char msg[128];
      snprintf(msg, 128, "row %d is not found", r);
      throw lemur::api::Exception("SparseMatrix::get(r)", msg);
    }
  }


  T& get(unsigned int r, unsigned int c) throw(lemur::api::Exception) {
    RowMapIt rit;
    typename Row<T>::ColMapIt cit;

    if((rit = _rows.find(r)) != _rows.end()) {
      if((cit = rit->second._cols.find(c)) != rit->second._cols.end()) {
        return cit->second;
      } else {
        char msg[128];
        snprintf(msg, 128, "column %d in row %d is not found", c, r);
        throw lemur::api::Exception("SparseMatrix::get(r,c)", msg);
      }
    } else {
      char msg[128];
      snprintf(msg, 128, "row %d is not found", r);
      throw lemur::api::Exception("SparseMatrix::get(r)", msg);
    }
  }

  const T& get(unsigned int r, unsigned int c) const throw(lemur::api::Exception) {
    cRowMapIt rit;
    typename Row<T>::cColMapIt cit;

    if((rit = _rows.find(r)) != _rows.end()) {
      if((cit = rit->second._cols.find(c)) != rit->second._cols.end()) {
        return cit->second;
      } else {
        char msg[128];
        snprintf(msg, 128, "column %d is not found", c);
        throw lemur::api::Exception("SparseMatrix::get(r,c)", msg);
      }
    } else {
      char msg[128];
      snprintf(msg, 128, "row %d is not found", r);
      throw lemur::api::Exception("SparseMatrix::get(r,c)", msg);
    }
  }

  bool exists(unsigned int r, unsigned int c, T*val) {
    RowMapIt rit;
    typename Row<T>::ColMapIt cit;

    if((rit = _rows.find(r)) != _rows.end()) {
      if((cit = rit->second._cols.find(c)) != rit->second._cols.end()) {
        if(val != NULL) {
          *val = cit->second;
        }
        return true;
      }
    }

    if(val != NULL) {
      *val = _def;
    }
    return false;
  }

  bool exists(unsigned int r, unsigned int c, T*val) const {
    cRowMapIt rit;
    typename Row<T>::cColMapIt cit;

    if((rit = _rows.find(r)) != _rows.end()) {
      if((cit = rit->second._cols.find(c)) != rit->second._cols.end()) {
        if(val != NULL) {
          *val = cit->second;
        }
        return true;
      }
    }

    if(val != NULL) {
      *val = _def;
    }
    return false;
  }

  bool exists(unsigned int r, Row<T>** row) {
    RowMapIt rit;
    if((rit = _rows.find(r)) != _rows.end()) {
      if(row != NULL) {
        *row = &rit->second;
      }
      return true;
    } else {
      if(row != NULL) {
        *row = NULL;
      }
      return false;
    }
  }

  size_t maxColIndex() const {
    cRowMapIt rit;
    typename Row<T>::crColMapIt cit;
    size_t max_col = 0;
    for(rit = _rows.begin(); rit != _rows.end(); rit++) {
      if(rit->second._cols.size()) {
        cit = rit->second._cols.rbegin();
        if(cit->first > max_col)
          max_col = cit->first;
      }
    }

    return max_col;
  }

  size_t maxRowIndex() const {
    crRowMapIt rit;
    if(!_rows.size())
      return 0;
    rit = _rows.rbegin();
    return rit->first;
  }

  inline size_t numRows() const {
    return _rows.size();
  }

  size_t maxNumColumns() const {
    size_t num_cols = 0;
    cRowMapIt rit;

    for(rit = _rows.begin(); rit != _rows.end(); rit++) {
      if(rit->second._cols.size() > num_cols)
        num_cols = rit->second._cols.size();
    }

    return num_cols;
  }

  void clear() {
    _rows.clear();
  }

  void deleteRow(unsigned int r) {
    _rows.erase(r);
  }

  void deleteCol(unsigned int c) {
    RowMapIt rit;
    typename Row<T>::ColMapIt cit;
    for(rit = _rows.begin(); rit != _rows.end(); rit++) {
      if((cit = rit->second._cols.find(c)) != rit->second._cols.end()) {
        rit->second._cols.erase(cit);
      }
    }
  }

  void setDef(T def) {
    _def = def;
  }

  void getDenseMatrix(T** &mat, size_t *nrows, size_t *ncols) {
    RowMapIt rit;
    typename Row<T>::ColMapIt cit;
    *nrows = numRows();
    *ncols = maxNumColumns();

    mat = new T*[*nrows];

    if(*nrows == 0 || *ncols == 0) {
      return;
    }

    for(unsigned int i = 0; i <= *nrows; i++) {
      mat[i] = new T[*ncols];
      for(unsigned int j = 0; j <= *ncols; j++) {
      }
    }
  }

  friend std::ostream &operator<<(std::ostream& stream, SparseMatrix<T> &m) {
    unsigned int r, c;
    typename SparseMatrix<T>::RowIterator rit(&m);
    for(r = rit.begin(); !rit.end(); r = rit.next()) {
      typename SparseMatrix<T>::ColIterator cit(&m, r);
      for(c = cit.begin(); !cit.end(); c = cit.next()) {
        stream << "[" << r << "," << c << "]:" << m[r][c] << " ";
      }
      stream << std::endl;
    }
  }

  class RowIterator;
  class ConstRowIterator;
  class ColIterator;
  class ConstColIterator;

protected:
  RowMap _rows;
  T _def;
};

template<class T> class SparseMatrix<T>::ColIterator {
public:
  ColIterator(Row<T>* r) : _row(r)
  { }

  ColIterator(SparseMatrix<T>* m, unsigned int i) throw(lemur::api::Exception) {
    typename SparseMatrix<T>::RowMapIt it;
    if((it = m->_rows.find(i)) == m->_rows.end()) {
      char msg[128];
      snprintf(msg, 128, "row %d is not found", i);
      throw lemur::api::Exception("ConstColIterator(m,r)", msg);
    } else {
      _row = &it->second;
    }
  }

  unsigned int begin() {
    _it = _row->_cols.begin();
    return _it->first;
  }

  bool end() {
    return _it == _row->_cols.end();
  }

  unsigned int next() {
    _it++;
    return _it->first;
  }

  unsigned int ind() const {
    return _it->first;
  }

  T& val() {
    return _it->second;
  }

protected:
  Row<T> *_row;
  typename Row<T>::ColMapIt _it;
};

template<class T> class SparseMatrix<T>::ConstColIterator {
public:
  ConstColIterator(const Row<T>* r) : _row(r)
  { }

  ConstColIterator(const SparseMatrix<T>* m, unsigned int i) throw(lemur::api::Exception) {
    typename SparseMatrix<T>::cRowMapIt it;
    if((it = m->_rows.find(i)) == m->_rows.end()) {
      char msg[128];
      snprintf(msg, 128, "row %d is not found", i);
      throw lemur::api::Exception("ConstColIterator(m,r)", msg);
    } else {
      _row = &it->second;
    }
  }

  unsigned int begin() {
    _it = _row->_cols.begin();
    return _it->first;
  }

  bool end() const {
    return _it == _row->_cols.end();
  }

  unsigned int next() {
    _it++;
    return _it->first;
  }

  unsigned int ind() const {
    return _it->first;
  }

  const T& val() const {
    return _it->second;
  }

protected:
  const Row<T> *_row;
  typename Row<T>::cColMapIt _it;
};

template<class T> class SparseMatrix<T>::ConstRowIterator {
public:
  ConstRowIterator(const SparseMatrix<T> *m) : _m(m)
  { }

  unsigned int begin() {
    _it = _m->_rows.begin();
    return _it->first;
  }

  bool end() const {
    return _it == _m->_rows.end();
  }

  unsigned int next() {
    _it++;
    return _it->first;
  }

  unsigned int ind() const {
    return _it->first;
  }

  const Row<T>& row() const {
    return _it->second;
  }

protected:
  const SparseMatrix<T> *_m;
  typename SparseMatrix<T>::cRowMapIt _it;
};

template<class T> class SparseMatrix<T>::RowIterator {
public:
  RowIterator(SparseMatrix<T> *m) : _m(m)
  { }

  unsigned int begin() {
    _it = _m->_rows.begin();
    return _it->first;
  }

  bool end() const {
    return _it == _m->_rows.end();
  }

  unsigned int next() {
    _it++;
    return _it->first;
  }

  unsigned int ind() const {
    return _it->first;
  }

  Row<T>& row() {
    return _it->second;
  }

protected:
  SparseMatrix<T> *_m;
  typename SparseMatrix<T>::RowMapIt _it;
};


#endif // SPARSE_MATRIX_HPP_
