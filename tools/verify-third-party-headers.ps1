# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$checks = @(
    @('vendor\ti\driverlib_port\dl_gpio.h', 'Texas Instruments'),
    @('vendor\ti\mspm0_sdk\source\ti\devices\msp\m0p\mspm0g350x.h', 'Texas Instruments'),
    @('vendor\arm\CMSIS\Core\Include\core_cm0plus.h', 'Apache-2.0'),
    @('firmware\drivers\imu\mpu6050\dmpKey.h', 'InvenSense')
)
foreach ($check in $checks) {
    $path = Join-Path $Root $check[0]
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing third-party file: $($check[0])" }
    if (-not ([IO.File]::ReadAllText($path).Contains($check[1]))) { throw "Missing upstream notice: $($check[0])" }
}
Write-Output 'Representative third-party notices are preserved.'
