# hw/arm/meson.build — i.MX RT1180 entries
# Appended to QEMU upstream hw/arm/meson.build by CI

imxrt1180_ss = ss.source_set()
imxrt1180_ss.add(when: 'CONFIG_IMXRT1180_SOC', if_true: files(
  'imxrt1180_soc.c',
  'imxrt1180_evk.c',
))
system_ss.add_all(when: 'CONFIG_IMXRT1180_SOC', if_true: imxrt1180_ss)
