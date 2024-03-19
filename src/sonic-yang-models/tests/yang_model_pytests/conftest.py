import os
import pytest
import yang as ly
from json import dumps
from glob import glob


class YangModel:

    def __init__(self) -> None:
        cur_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.abspath(os.path.join(cur_dir, '..', '..'))
        self.model_dir = os.path.join(project_root, 'yang-models')

        self._load_model()

    def _load_model(self) -> None:
        self.ctx = ly.Context(self.model_dir)
        yang_files = glob(self.model_dir +"/*.yang")

        for file in yang_files:
            m = self.ctx.parse_module_path(file, ly.LYS_IN_YANG)
            if not m:
                raise RuntimeError("Failed to parse '{file}' model")

    def _load_data(self, data) -> None:
        self.ctx.parse_data_mem(dumps(data), ly.LYD_JSON,
                                ly.LYD_OPT_CONFIG | ly.LYD_OPT_STRICT)

    def load_data(self, data, expected_error=None) -> None:
        if expected_error:
            with pytest.raises(RuntimeError) as exc_info:
                self._load_data(data)

            assert expected_error in str(exc_info)
        else:
            self._load_data(data)


@pytest.fixture
def yang_model():
    yang_model = YangModel()
    return yang_model
