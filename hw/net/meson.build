# hw/net/meson.build entries for i.MX RT1180

imxrt1180_net_ss = ss.source_set()
imxrt1180_net_ss.add(when: 'CONFIG_IMXRT1180_SOC', if_true: files(
    'imxrt1180_enet.c',
    'imxrt1180_dp83822_phy.c',
))
system_ss.add_all(when: 'CONFIG_IMXRT1180_SOC', if_true: imxrt1180_net_ss)
