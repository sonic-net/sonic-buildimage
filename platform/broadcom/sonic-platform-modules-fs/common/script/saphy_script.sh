#!/bin/bash

start() {
    # 获取当前device目录
    BDIR="/usr/share/sonic/device"
    CURDEV="$(cat /host/machine.conf | grep onie_platform)"
    array=(${CURDEV//=/ })
    PLTF=${array[1]}
    DEVDIR=${BDIR}"/"${PLTF}
    def_sku=${DEVDIR}"/default_sku"
    cur_sku=${DEVDIR}"/current_sku"
    SKU=""
    if [ ! -e "/usr/local/bin/saphy" ]; then
        exit 0
    fi
    if type config-hwsku.sh >/dev/null 2>&1; then
        SKU="$(config-hwsku.sh -p)"
        echo "config-hwsku.sh get sku:"${SKU}
    else
        if test -e "$cur_sku"; then
            sku=${cur_sku}
            echo "cur_sku:"${sku}
        elif test -e "$def_sku"; then
            sku=${def_sku}
            echo "def_sku:"${sku}
        else
            echo "sku file not find !!!"
            exit
        fi
        t="$(cat ${sku})"
        array=(${t// / })
        SKU=${array[0]}
    fi
    hwsku_dir="/usr/share/sonic/hwsku"
    target_path="${BDIR}/${PLTF}/${SKU}"
    # 检查hwsku软链接
    if [ -d "$hwsku_dir" ]; then
        if [ -L "$hwsku_dir" ]; then
            linked_path=$(readlink "$hwsku_dir")
            if [ "$linked_path" != "$target_path" ]; then
                rm "$hwsku_dir"
                ln -s "$target_path" "$hwsku_dir"
            fi
        fi
    else
        ln -s "$target_path" "$hwsku_dir"
    fi
    # 启动saphy进程
    echo "Starting /usr/local/bin/saphy..."
    /usr/local/bin/saphy service
}

wait() {
    echo "wait /usr/local/bin/saphy..."
}

stop() {
    echo "Stopping /usr/local/bin/saphy..."
    kill `ps -ef | grep /usr/local/bin/saphy | awk '{print $2}'`
    echo "Stopped /usr/local/bin/saphy..."
    exit 0
}

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
