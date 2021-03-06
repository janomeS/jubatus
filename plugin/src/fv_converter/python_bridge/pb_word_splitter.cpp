// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2017 Preferred Networks and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "pb_word_splitter.hpp"

#include <Python.h>

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "python_bridge.hpp"

namespace jubatus {
namespace plugin {
namespace fv_converter {
namespace python {

void pb_word_splitter::split(
    const std::string& text,
    std::vector<std::pair<size_t, size_t> >& result) const {
  scoped_gil lk;

  pb_object ptext(pb_unicode_from_string(text));
  PB_CHECK(ptext,
           "cannot convert input text to Python object: " << text);

  pb_object ret(PyObject_CallMethodObjArgs(
      ins_.get(),
      method_.get(),
      ptext.get(),
      NULL));
  PB_CHECK(ret,
           name_ << " method cannot be called");
  PB_CHECK(PyList_CheckExact(ret.get()),
           name_ << " method returned non-list type: " << pb_str(ret.get()));

  size_t size = PyList_Size(ret.get());
  for (size_t i = 0; i < size; ++i) {
    PyObject* tpl = PyList_GetItem(ret.get(), i);

    PB_CHECK(tpl,
             "item " << i << " cannot be accessed: "
             << pb_str(ret.get()));
    PB_CHECK(PyTuple_CheckExact(tpl),
             "list must not contain non-tuple: " << pb_str(tpl));
    PB_CHECK(PyTuple_Size(tpl) == 2,
             "tuple length must be 2: " << pb_str(tpl));

    PyObject* begin = PyTuple_GetItem(tpl, 0);
    PyObject* length = PyTuple_GetItem(tpl, 1);

    PB_CHECK(PyNumber_Check(begin),
             "begin must be a number: " << pb_str(tpl));
    PB_CHECK(PyNumber_Check(length),
             "length must be a number: " << pb_str(tpl));

    pb_object begin2(PyNumber_Long(begin));
    PB_CHECK(begin2,
             "begin cannot be converted as long: " << pb_str(tpl));
    pb_object length2(PyNumber_Long(length));
    PB_CHECK(length2,
             "length cannot be converted as long: " << pb_str(tpl));

    result.push_back(std::make_pair(
        PyLong_AsSsize_t(begin2.get()),
        PyLong_AsSsize_t(length2.get())));
  }
}

extern "C" {
jubatus::core::fv_converter::word_splitter* word_splitter(
    const std::map<std::string, std::string>& params) {
  initialize();
  return new pb_word_splitter(setup(params));
}
}  // extern "C"

}  // namespace python
}  // namespace fv_converter
}  // namespace plugin
}  // namespace jubatus
