# Smart Dumbbell 团队仓库使用说明

适用仓库：[HansonHe-UW/ece198-smart-dumbbell](https://github.com/HansonHe-UW/ece198-smart-dumbbell)

这份说明规定小组怎样存放文件、分工、修改设计、提交代码和记录测试。日常按一个流程工作：**先更新 → 认领一个任务 → 新建分支 → 修改和验证 → 提交 PR → 队友检查 → 合并 → 再更新。**

当前仓库处于概念和规划阶段，已有 specification、CAD、渲染图、PPT 和讲稿。`firmware/`、`software/`、`ml/` 目前只是工作入口，没有完成的软件或训练结果。

## 1. 先认识这几个词

| 名称 | 在本项目中是什么意思 |
|---|---|
| Repository / repo | 项目文件和修改历史，不只是一个网盘 |
| `main` | 小组已经检查并接受的当前版本；不代表所有硬件功能已经实现 |
| Branch / 分支 | 为一个任务建立的修改线路 |
| Commit / 提交 | 在本地保存一组有说明的修改 |
| Push / 推送 | 把本地提交上传到 GitHub |
| Pull / 拉取 | 把远程修改同步到本地 |
| Issue | 一项任务或一个问题，写清负责人、目标和完成条件 |
| Pull request / PR | 请求小组检查并把一个分支的修改合并到 `main` |
| Merge / 合并 | 把已接受的修改纳入主版本 |

**保存文件、commit、push、merge 是四个不同步骤。** 文件在你的电脑上保存了，不等于其他人已经能看到；分支 push 了，也不等于已经进入 `main`。

## 2. 第一次加入

### 仓库拥有者

1. 向队友收集 GitHub 用户名。
2. 在仓库的 **Settings → Collaborators** 中邀请队友。
3. 队友接受邀请后，才有直接向这个仓库推送分支的权限。

公开仓库允许别人查看，不代表所有人都能直接改写。不要共享个人 GitHub 密码或 token。邀请步骤见 [GitHub 官方说明](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/repository-access-and-collaboration/inviting-collaborators-to-a-personal-repository)。本说明和模板不会自动添加成员，也不会自动启用分支保护。

### 每位队友

1. 登录自己的 GitHub 账号，接受协作邀请。
2. 先读根目录 `README.md`、本说明和 `docs/specification.md`。
3. 代码、CAD 和 PowerPoint 工作建议克隆仓库；只有简单文字修改时也可使用网页。
4. 每个人维护自己的本地副本，不在同一个共享同步文件夹中同时操作 `.git`。

在自己选择的项目父目录打开终端，然后执行：

```bash
git clone https://github.com/HansonHe-UW/ece198-smart-dumbbell.git
cd ece198-smart-dumbbell
git status
```

Hanson 已经有一个本地副本，位置是桌面的 `26FALL/ECE 198/smart-dumbbell`，不必再克隆一次。以后以仓库里的文件为工作文件；仓库外原有的 `ECE 198` 文件不会自动同步进 GitHub。

如果 Git 要求设置提交身份，请在仓库内配置自己的名字和邮箱：

```bash
git config user.name "Your Name"
git config user.email "YOUR_GITHUB_NOREPLY_EMAIL"
```

把示例值替换成自己的信息。若希望提交记录不公开个人邮箱，使用 GitHub **Settings → Emails** 显示的 noreply 地址。GitHub 登录用于推送，Git 提交身份用于作者记录，两者不是同一个设置。

## 3. 文件放哪里，发生冲突时听谁的

| 内容 | 位置 | 使用规则 |
|---|---|---|
| 项目入口与状态 | `README.md` | 简要解释项目，链接当前文件 |
| 当前规格 | `docs/specification.md` | 英文工作基线；功能、接口、验收条件以它为准 |
| 中文规格 | `docs/specification-zh.md` | 与英文版同步更新；发现差异要修复，不能保留两个冲突要求 |
| 设计决定 | `docs/decisions.md` | 记录为什么改变范围、硬件或尺寸 |
| 硬件与预算 | `docs/hardware.md` | 型号、数量、借用状态、实物尺寸、价格日期 |
| 调研 | `docs/research-plan.md` 及后续调研文件 | 写原始链接、发现和对设计的影响 |
| PDF 规格快照 | `docs/exports/` | 给人阅读的导出版本；不是独立维护的另一份规格 |
| CAD 与结构说明 | `hardware/cad/` | 模型、生成脚本和尺寸假设保持一致 |
| 渲染图 | `hardware/renders/` | 外观示意，不能当作精确尺寸图 |
| PPT 和讲稿 | `presentation/` | 对外解释设计，要与当前规格一致 |
| 微控制器程序 | `firmware/` | 传感器、计数、按钮、屏幕、通信等 |
| 电脑程序 | `software/` | 设置目标、接收数据、显示与导出 |
| 机器学习 | `ml/` | 数据处理、训练、模型、评估说明 |
| 测试记录 | `tests/` | 测试方法、版本、实际结果与限制 |
| 数据管理说明 | `data/` | 原始录制和私人数据默认不进入公开仓库 |

**修改功能先改规格；修改硬件同步改 BOM 和决定记录；最后再更新 PDF、PPT 和讲稿。** 口头讨论和微信群文件不会自动成为仓库的新基线，形成结论后要提交。

目前 80 mm 端部只是讨论中的候选尺寸。已提交的 CAD v0.1 仍为最大宽度 100 mm；不要把候选尺寸写成已经完成的模型。

## 4. 先认领任务，避免互相覆盖

在 **Issues → New issue → Task** 创建任务，至少写清：

- 要解决什么问题，涉及哪些文件或模块。
- 一位负责人和一位检查人。
- 预计完成日期。
- 怎样判断完成，关联哪个 `FR-xx` 或 `AT-xx`。
- 有没有等待器材、决定或他人接口。

任务状态在正文使用：`Todo / In progress / In review / Done / Blocked`。不依赖预先配置的标签。软件任务通常在 PR 合并后关闭；实验任务需要有结果记录才能关闭。

建议工作区域如下，具体负责人由小组认领，不自动等同于已分配：

| 工作区域 | 典型任务 |
|---|---|
| Electronics and power | 器件确认、电平、接线、电源、光学接触 |
| Firmware | 采样、计数、显示、按钮、Wi-Fi |
| ML and data | 采集协议、标签、训练、独立评估 |
| Enclosure and computer interface | CAD 固定结构、装配、电脑记录界面 |

一个任务尽量只解决一个明确问题。例如“记录 ADXL335 三轴数据”比“完成整个哑铃代码”更适合作为一个任务。

## 5. 修改文档的网页流程

不熟悉 Git 的成员可以先按这个方式做文字调研：

1. 打开仓库，确认当前查看的是 `main`。
2. 用分支下拉菜单，从 `main` 创建如 `docs/sensor-research` 的分支。
3. 确认页面已切换到该分支，再打开目标 Markdown 文件，点编辑。
4. 修改后选择 **Commit changes**，写一句具体说明，把修改提交到当前任务分支。
5. 点击 **Compare & pull request**；确认 base 是 `main`，compare 是自己的分支。
6. 按 PR 模板填写，交给另一位队友检查。

上传 PPT、PDF、DWG 也应在任务分支上完成。提交前确认目标文件夹和文件名。较大的二进制文件或批量改动更适合本地 Git 流程。

## 6. 本地工作的完整流程

以下以更新硬件文档为例。分支名和文件路径需按实际任务调整。

### 开始工作

```bash
git status
git switch main
git pull --ff-only
git switch -c docs/sensor-research
```

前提：`git status` 没有尚未处理的本地修改。若已有未提交的工作，先留在自己的任务分支保存提交，不要用强制切换或 `reset --hard` 清掉它。`pull --ff-only` 若失败，先检查原因，不要直接强制覆盖。

### 修改、检查、提交

用编辑器修改文件，然后执行：

```bash
git diff
git add docs/hardware.md
git diff --cached
git commit -m "docs: confirm motion sensor availability"
git push -u origin docs/sensor-research
```

`git add` 后面列自己这次确实需要提交的文件。先看 `git diff --cached`，确认没有误带日志、私人资料或无关修改。以上 commit 说明仅是例子，不能在尚未确认传感器时写成已经确认。

### 创建 PR

到 GitHub 页面点击 **Compare & pull request**。一个 PR 至少说明：

- 改了什么，为什么改。
- 关联哪个任务和需求。
- 如何检查，什么尚未测试。
- 是否改变接口、器材、预算、尺寸或验收标准。

未做完可设为 **Draft**，不要把 Draft 当成已经可合并的版本。

### 分支开发期间同步别人修改

先提交自己的工作，再执行：

```bash
git fetch origin
git merge origin/main
```

若出现冲突，按下一节处理。这里采用 merge，初期不要求大家学习 rebase，也不需要 force push。

### 合并后更新本地

```bash
git switch main
git pull --ff-only
git branch -d docs/sensor-research
```

最后一行只用于删除已合并的本地任务分支。若 Git 拒绝删除，先保留分支并检查；不要直接改用 `-D`。远程分支可在已合并的 PR 页面点 **Delete branch**。

## 7. PR 检查与合并约定

- 日常修改通过 PR 进入 `main`，至少由一名非作者队友检查。
- 检查人看实际差异和验证证据，而不只看“看起来可以”。
- 修改规格、预算或器材时，同时检查相关文档和演示材料是否一致。
- 维护者或本次任务约定的合并负责人执行合并。初期默认使用 **Create a merge commit / Merge pull request**，便于保留原提交关系。
- 合并前再次确认没有冲突，也没有把未通过的测试描述为通过。
- 这些是团队约定。是否由 GitHub 自动强制执行，取决于另行配置的保护规则，本文不会自动开启规则。

本次仓库初始化和首次建立协作说明属于启动工作，可由维护者直接发布；后续日常开发使用上述 PR 流程。用户的明确指令可以调整流程，但应保留修改说明。

## 8. CAD、PPT 和 PDF 怎么协作

这些文件不适合像代码一样自动合并，所以同一个版本文件同一时间只安排一个人修改。

1. 在任务单中说明“由谁修改哪个文件”，其他人先提出意见，不同时改同一个二进制文件。
2. 从最新 `main` 的版本开始修改。
3. 用清晰版本名，例如 `Smart_Dumbbell_Concept_v02.dwg`、`Smart_Dumbbell_Pitch_Waterloo_v03.pptx`，不要用 `final_final_new2`。
4. 在 PR 中附预览或列出具体变化；截图不替代可编辑源文件。
5. CAD 改尺寸时同步 `MODEL_NOTES.txt` 和生成脚本；脚本若尚不能重建新版本，要明确标注。
6. PPT 改叙述时同步对应讲稿，重新计时；PDF 导出要注明对应规格版本。
7. 接受新版本后更新 README 的“当前文件”链接，避免队友继续使用旧版。

渲染图上的螺丝、比例或屏幕细节可能只是概念表达；实际加工应依据已检查的 CAD 和实物尺寸。

## 9. 代码、接口和测试怎么交接

### 代码提交

在相应模块 README 中记录所用工具版本、依赖、运行方式，以及能重现的测试步骤。首次接线还要记录模块型号、电平、引脚和安装方向。

示例任务分支：

```text
firmware/adxl335-readout
firmware/curl-counter
ml/curl-idle-other
software/session-logger
hardware/grip-contact-test
docs/hardware-check
```

修改 Wi-Fi 数据字段、单位、时间戳或无效值处理时，同一个 PR 要更新规格中的接口定义，并说明发送端和接收端如何同时适配。接口尚未实现时可以明确标成 proposal，不能假装另一端已经支持。

### 测试记录

从 [测试记录模板](../tests/templates/test-record.md) 复制一个文件到 `tests/results/`，以日期和测试名命名。记录 commit、硬件和模型版本、步骤、数据来源、预期标准、实际结果以及限制。

例如：

```text
tests/results/2026-10-20_adxl335_readout.md
```

只有实际做过并有证据的测试才能写 `Pass`。编译通过、模型离线评估、模拟器通过和真实硬件通过是不同层面的证据。CAD 的 AUDIT 通过也不代表结构强度或装配间隙已经验证。

模型训练必须先按人员或完整录制会话划分训练与测试数据，再切窗口；不能把相邻重叠片段随机分进两边。心率测试要同时报告有效覆盖率与误差。

## 10. 公开仓库的文件边界

可以提交：项目代码、已审核的规格、BOM、CAD、演示文件、匿名且获准公开的测试结果、明确标注的合成数据。

不要提交：账号密码、token、Wi-Fi 密码、私人联系表、真实参与者身份或健康资料、未经允许公开的课程原始材料、无关个人文件。

`data/raw/`、`data/private/` 和 `data/session_logs/` 已默认忽略。**`.gitignore` 不会保护已经提交过的文件，也不会清除历史。** 不要为了让上传成功而随意 `git add -f`。

如果误推送凭据，先撤销或更换凭据并通知仓库拥有者；仅删除最新文件不够。若误推送个人资料，停止继续传播，和相关人员协调处理。

## 11. 常见问题

| 情况 | 处理方式 |
|---|---|
| Push 显示没有权限 | 确认登录的是自己的账号，已接受 collaborator 邀请，remote 指向正确仓库 |
| `git status` 有本地修改但想换分支 | 先在任务分支保存提交；不要强制覆盖或丢弃工作 |
| `pull --ff-only` 失败 | 说明需要检查本地和远程历史；先 `git status`、`git log --oneline --graph -8`，不要直接 force push |
| Markdown／代码冲突 | 比较两边意图，编辑成正确版本，移除冲突标记，验证后 `git add` 和 `git commit`；不会处理时请队友一起看 |
| 想取消刚开始的冲突合并 | 使用 `git merge --abort` 回到合并前，再协调处理 |
| DWG／PPTX 冲突 | 找文件负责人，在应用中人工整理出一个版本；不要凭文件日期覆盖另一人的工作 |
| 提交后队友仍看不到 | 检查是否已经 push，以及对方查看的是你的分支还是 `main` |
| 文件上传了但不在主页当前版本 | 检查 PR 是否合并；README 是否链接了正确版本 |
| 出错版本已经合并 | 用一个新提交或修复 PR 恢复正确行为，保留历史；不要重写公共 `main` |

## 12. 每次结束工作前的检查

- [ ] 任务目标和负责人清楚。
- [ ] 已同步最新基线，使用自己的任务分支。
- [ ] 只提交本次需要的文件。
- [ ] 规格、BOM、接口、CAD、讲稿中受影响的内容已同步，或明确列为待跟进。
- [ ] 写清实际测试和未验证部分。
- [ ] 没有凭据或未经允许公开的个人数据。
- [ ] 已 push，PR 的目标分支是 `main`。
- [ ] 队友知道需要检查什么。

日常分支和 PR 流程参考 [GitHub flow](https://docs.github.com/en/get-started/using-github/github-flow)。
