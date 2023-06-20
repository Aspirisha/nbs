import os
import logging
import yatest.common as common
import time
import tarfile

from .common import get_mount_paths
from .qemu import Qemu

EMU_NET = "10.0.2.0/24"
QEMU_HOST = "10.0.2.2"

logger = logging.getLogger(__name__)


def _get_bindir():
    return common.build_path(
        "cloud/storage/core/tools/testing/qemu/bin")


def _unpack_qemu_bin(bindir):
    with tarfile.open(os.path.join(bindir, "qemu-bin.tar.gz")) as tf:
        tf.extractall(bindir)


def _get_qemu_kvm():
    bindir = _get_bindir()
    qemu_kvm = os.path.join(bindir, "usr", "bin", "qemu-system-x86_64")
    if not os.path.exists(qemu_kvm):
        _unpack_qemu_bin(bindir)

    return qemu_kvm


def _get_qemu_firmware():
    bindir = _get_bindir()
    qemu_firmware = os.path.join(bindir, "usr", "share", "qemu")
    if not os.path.exists(qemu_firmware):
        _unpack_qemu_bin(bindir)

    return qemu_firmware


class QemuWithMigration:
    def __init__(self, socket_generator):
        self.qemu = Qemu(
            qemu_kmv=_get_qemu_kvm(),
            qemu_firmware=_get_qemu_firmware(),
            rootfs=common.build_path("cloud/storage/core/tools/testing/qemu/image/rootfs.img"),
            kernel=None,
            kcmdline=None,
            initrd=None,
            mem="4G",
            proc=8,
            virtio='fs',
            qemu_options=[],
            vhost_socket="",
            enable_kvm=True)

        self.socket_generator = socket_generator

    def start(self):
        self.socket = self.socket_generator(0, False)

        self.qemu.set_mount_paths(get_mount_paths())
        self.qemu.set_vhost_socket(self.socket)
        self.qemu.start()

    def migrate(self, count, timeout):
        for migration in range(0, count):
            self.socket = self.socket_generator(migration, False)
            self.qemu.migrate(migration, self.socket)
            time.sleep(timeout)
