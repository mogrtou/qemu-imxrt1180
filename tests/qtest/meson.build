# tests/qtest/meson.build — i.MX RT1180 qtest 构建规则
#
# 此文件注册 imxrt1180-* qtest 可执行文件。
# 它被 qemu/tests/qtest/meson.build 通过 subdir() 调用引用。
# 实际引用行需要在 qemu/tests/qtest/meson.build 中添加:
#   subdir('imxrt1180')

qtests_imxrt1180 = [
  'imxrt1180-enet-test',
]

foreach test : qtests_imxrt1180
  test_exe = executable(
    test,
    sources: files(test + '.c'),
    dependencies: [qemuutil, qtest],
    c_args: '-DQEMU_BUILD=1',
    build_by_default: false,
  )
  test(test, test_exe,
    depends: [qemu_softmmu],
    env: qtest_env,
    suite: ['imxrt1180'],
  )
endforeach
