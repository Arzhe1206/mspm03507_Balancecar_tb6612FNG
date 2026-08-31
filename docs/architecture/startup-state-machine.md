> 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
>
> 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car · 项目标识：MSPM0-BC-ARZHE1206
>
> AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。

# Startup State Machine

```text
POWER ON
   │
   v
DMP LOADING ── failure ──> FAULT
   │
   v
LIVE ANGLES / KEEP STILL
   │  Z drift stable for 2 s
   v
COLLECT 200 FRESH SAMPLES
   │
   v
PRESS KEY TO ZERO ── 3 s timeout ──> AUTO ZERO
   │                                   │
   └─────────────────┬─────────────────┘
                     v
             RESET PID / ENCODER
                     │ 2 valid frames
                     v
                  RUNNING
                     │ 5 s
                     v
        TRACE KEY ALLOWED IF MACRO ON
```

任何软件急停或需要重新标定的故障都会返回安全流程，不沿用旧零点。
