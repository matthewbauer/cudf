# Copyright (c) 2020, NVIDIA CORPORATION.

import pickle

import numpy as np
import pandas as pd
import pyarrow as pa
from pandas.api.extensions import ExtensionDtype

import cudf


class CategoricalDtype(ExtensionDtype):
    def __init__(self, categories=None, ordered=None):
        """
        dtype similar to pd.CategoricalDtype with the categories
        stored on the GPU.
        """
        self._categories = self._init_categories(categories)
        self.ordered = ordered

    @property
    def categories(self):
        if self._categories is None:
            return cudf.core.index.as_index(
                cudf.core.column.column_empty(0, dtype="object", masked=False)
            )
        return cudf.core.index.as_index(self._categories)

    @property
    def type(self):
        return self._categories.dtype.type

    @property
    def name(self):
        return "category"

    @property
    def str(self):
        return "|O08"

    @classmethod
    def from_pandas(cls, dtype):
        return CategoricalDtype(
            categories=dtype.categories, ordered=dtype.ordered
        )

    def to_pandas(self):
        if self.categories is None:
            categories = None
        else:
            categories = self.categories.to_pandas()
        return pd.CategoricalDtype(categories=categories, ordered=self.ordered)

    def _init_categories(self, categories):
        if categories is None:
            return categories
        if len(categories) == 0:
            dtype = "object"
        else:
            dtype = None

        column = cudf.core.column.as_column(categories, dtype=dtype)

        if isinstance(column, cudf.core.column.CategoricalColumn):
            return column.categories
        else:
            return column

    def __eq__(self, other):
        if isinstance(other, str):
            return other == self.name
        elif other is self:
            return True
        elif not isinstance(other, self.__class__):
            return False
        elif self.ordered != other.ordered:
            return False
        elif self._categories is None or other._categories is None:
            return True
        else:
            return (
                self._categories.dtype == other._categories.dtype
                and self._categories.equals(other._categories)
            )

    def construct_from_string(self):
        raise NotImplementedError()

    def serialize(self):
        """
        Converts the CategoricalDtype into a header and list of
        Buffer/memoryview objects for file storage or
        network transmission.

        Returns
        -------
            header : dictionary containing any serializable metadata
            frames : list of Buffer or memoryviews, commonly of length one

        :meta private:
        """
        header = {}
        frames = []
        header["ordered"] = self.ordered
        if self.categories is not None:
            categories_header, categories_frames = self.categories.serialize()
        header["categories"] = categories_header
        frames.extend(categories_frames)
        return header, frames

    @classmethod
    def deserialize(cls, header, frames):
        """Convert serialized header and frames back
        into CategoricalDtype object

        Parameters
        ----------
        cls : class of object
        header : dict
            dictionary containing any serializable metadata
        frames : list of Buffer or memoryview objects

        Returns
        -------
        Deserialized CategoricalDtype extracted
        from frames and header

        :meta private:
        """
        ordered = header["ordered"]
        categories_header = header["categories"]
        categories_frames = frames
        categories_type = pickle.loads(categories_header["type-serialized"])
        categories = categories_type.deserialize(
            categories_header, categories_frames
        )
        return cls(categories=categories, ordered=ordered)


class ListDtype(ExtensionDtype):
    def __init__(self, element_type):
        if isinstance(element_type, ListDtype):
            self._typ = pa.list_(element_type._typ)
        else:
            element_type = cudf.utils.dtypes.np_to_pa_dtype(
                np.dtype(element_type)
            )
            self._typ = pa.list_(element_type)

    @property
    def element_type(self):
        if isinstance(self._typ.value_type, pa.ListType):
            return ListDtype.from_arrow(self._typ.value_type)
        else:
            return np.dtype(self._typ.value_type.to_pandas_dtype()).name

    @property
    def leaf_type(self):
        if isinstance(self.element_type, ListDtype):
            return self.element_type.leaf_type
        else:
            return self.element_type

    @property
    def type(self):
        # TODO: we should change this to return something like a
        # ListDtypeType, once we figure out what that should look like
        return pa.array

    @property
    def name(self):
        return "list"

    @classmethod
    def from_arrow(cls, typ):
        obj = object.__new__(cls)
        obj._typ = typ
        return obj

    def to_arrow(self):
        return self._typ

    def to_pandas(self):
        super().to_pandas(integer_object_nulls=True)

    def __eq__(self, other):
        if isinstance(other, str):
            return other == self.name
        if type(other) is not ListDtype:
            return False
        return self._typ.equals(other._typ)

    def __repr__(self):
        if isinstance(self.element_type, ListDtype):
            return f"ListDtype({self.element_type.__repr__()})"
        else:
            return f"ListDtype({self.element_type})"
