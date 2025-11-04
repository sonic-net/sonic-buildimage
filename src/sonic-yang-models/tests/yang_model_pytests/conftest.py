import os
import pytest
import libyang as ly
from json import dumps
from glob import glob

class YangModel:

    def __init__(self) -> None:
        cur_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.abspath(os.path.join(cur_dir, '..', '..'))
        self.model_dir = os.path.join(project_root, 'yang-models')
        self._load_model()

    def __del__(self) -> None:
        self.ctx.destroy()
        self.ctx = None

    def _load_model(self) -> None:
        self.ctx = ly.Context(self.model_dir)
        yang_files = glob(self.model_dir +"/*.yang")
        for file in yang_files:
            with open(file, 'r') as f:
                m = self.ctx.parse_module_file(f, "yang")
                if not m:
                    raise RuntimeError("Failed to parse '{file}' model")

    def _load_data(self, data) -> None:
        dnode = self.ctx.parse_data_mem(dumps(data), "json", strict=True, no_state=True, json_string_datatypes=True)
        dnode.free()

    def load_data(self, data, expected_error=None) -> None:
        if expected_error:
            with pytest.raises(Exception) as exc_info:
                self._load_data(data)
            assert expected_error in str(exc_info)
        else:
            self._load_data(data)


@pytest.fixture
def yang_model():
    yang_model = YangModel()
    return yang_model
