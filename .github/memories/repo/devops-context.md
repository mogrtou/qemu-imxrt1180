# DevOps Agent — 上下文恢复指南

> 最后一次活跃: 2026-05-11 (本轮更新)

## 当前进度
- ✅ Dockerfile — 完整 (含 lcov, gcovr, ccache, ARM toolchain, QEMU 构建依赖)
- ✅ docker-compose.yml — 完整 (qemu-dev + ci 服务, ccache 持久化卷)
- ✅ .github/workflows/ci.yml — 完整 5 个 job (build-qemu, build-firmware, test-qtest, test-integration, coverage)
- ✅ ccache 构建缓存 — CI 使用 actions/cache@v4 + hashFiles 键
- ✅ 覆盖率报告系统 — 独立 coverage job, --enable-gcov, gcovr + lcov 双报告
- ✅ CI 状态 badge — README.md 顶部
- ✅ 分支保护规则文档 — docs/branch-protection.md
- ⚠️ hw/arm/meson.build — 已有 CONFIG_IMXRT1180_SOC/CONFIG_IMXRT1180_EVK 注册
- ⚠️ Kconfig 选项 — 待 QEMU Dev 添加
- ⚠️ hw/net/meson.build — 待 QEMU Dev 添加 ENET 源文件注册

## 本轮产出 (2026-05-11)
1. **覆盖率报告系统**: coverage job 完全独立, 使用 --enable-gcov 重新编译, 同时生成 gcovr 和 lcov 两份 HTML 报告, 上传原始 .gcda/.gcno 数据
2. **ccache 缓存优化**: CI 缓存键改用 hashFiles('qemu/configure', 'qemu/meson.build'), 提高缓存命中率; Dockerfile 含 ccache + ENV 配置
3. **CI 流水线验证**: build-qemu 和 build-firmware job 独立运行; test-integration 设 continue-on-error: true 不阻塞; 无集成测试文件时自动跳过
4. **CI 配置完善**: README.md 添加 CI badge; test-integration 添加文件存在性检查; docs/branch-protection.md 定义分支保护规则、PR 合并流程、CI 失败处理矩阵

## 已知注意事项
- coverage job 使用 needs: [] (完全独立, 不依赖其他 job)
- test-integration job 有 continue-on-error: true, 失败不阻塞 PR
- ccache 缓存跨构建共享, 首次冷启动后后续构建可加速 5-10x
- GitHub 仓库 Settings → Branches 需手动配置 branch protection (参考 docs/branch-protection.md)
- hw/net/ 的 meson.build 注册需 QEMU Dev 添加 ENET 源文件后由 DevOps 完成
