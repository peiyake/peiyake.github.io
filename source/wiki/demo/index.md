---
layout: wiki  # 使用wiki布局模板
wiki: Demo # 这是项目名
title: wiki使用
order: 1
---

## Stellar如何使用`wiki`

我觉得当你想对某个**专题**的东西（技术、概念、理论、项目等）进行一个完成的总结、记录时，适合使用 **wiki**。

### 第一步

启用**wiki**侧边栏,修改配置`blog/_config.stellar.yml`，添加**wiki**的导航

```
sidebar:
  menu:
    post: '[btn.blog](/)'
    about: '[btn.about](/about/)'
    wiki: '[btn.wiki](/wiki/)'
```

### 第二步

新建文件夹

```
blog/source/wiki
```

新建项目文件夹


```
blog/source/wiki/demo
```

新建项目索引文件

```
blog/source/wiki/demo/index.md
```

index.md

```
---
layout: wiki  # 使用wiki布局模板
wiki: Demo # 这是项目名
---
```

### 第三步

新建项目配置文件 

```
blog/source/_data/projects.yml
```

projects.yml

```
Demo:
  title: 基于Stallar的wiki示例
  subtitle: 一个wiki搭建的例子
  tags: [专栏示例]
  cover: true
  logo:
    src: https://fastly.jsdelivr.net/gh/cdn-x/wiki@1.0.2/stellar/icon.svg
    small: 112px
    large: 240px
  description: Stallar是一个非常好用的的hexo主题，尤其是它的wiki功能，让你可以把零散的文章制作成一个专栏。本文就简要介绍了wiki的搭建方法
```

