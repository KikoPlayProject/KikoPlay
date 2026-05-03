(function () {
    'use strict';

    // --- Theme management ---
    var root = document.documentElement;

    function darkenColor(hex, factor) {
        var r = parseInt(hex.slice(1, 3), 16);
        var g = parseInt(hex.slice(3, 5), 16);
        var b = parseInt(hex.slice(5, 7), 16);
        r = Math.round(r * factor);
        g = Math.round(g * factor);
        b = Math.round(b * factor);
        return '#' + [r, g, b].map(function (c) {
            return c.toString(16).padStart(2, '0');
        }).join('');
    }

    function applyAccent(color) {
        root.style.setProperty('--accent', color);
        root.style.setProperty('--accent-dim', darkenColor(color, 0.6));
    }

    function initTheme() {
        var theme = localStorage.getItem('kiko-theme') || 'dark';
        var accent = localStorage.getItem('kiko-accent') || '#4fc3f7';
        root.setAttribute('data-theme', theme);
        applyAccent(accent);
        updateThemeIcon(theme);
        updateColorDots(accent);
        document.getElementById('custom-color').value = accent;
    }

    function updateThemeIcon(theme) {
        var sun = document.querySelector('.icon-sun');
        var moon = document.querySelector('.icon-moon');
        if (theme === 'light') {
            sun.style.display = 'none';
            moon.style.display = '';
        } else {
            sun.style.display = '';
            moon.style.display = 'none';
        }
    }

    function updateColorDots(accent) {
        var dots = document.querySelectorAll('.color-dot');
        dots.forEach(function (dot) {
            dot.classList.toggle('active', dot.dataset.color === accent);
        });
    }

    document.getElementById('theme-toggle').addEventListener('click', function () {
        var current = root.getAttribute('data-theme') || 'dark';
        var next = current === 'dark' ? 'light' : 'dark';
        root.setAttribute('data-theme', next);
        localStorage.setItem('kiko-theme', next);
        updateThemeIcon(next);
    });

    var colorPopup = document.getElementById('color-popup');
    document.getElementById('color-toggle').addEventListener('click', function (e) {
        e.stopPropagation();
        colorPopup.classList.toggle('open');
    });

    document.addEventListener('click', function (e) {
        if (!e.target.closest('.theme-color-wrap')) {
            colorPopup.classList.remove('open');
        }
    });

    colorPopup.addEventListener('click', function (e) {
        var dot = e.target.closest('.color-dot');
        if (!dot) return;
        var color = dot.dataset.color;
        applyAccent(color);
        localStorage.setItem('kiko-accent', color);
        updateColorDots(color);
        document.getElementById('custom-color').value = color;
    });

    document.getElementById('custom-color').addEventListener('input', function (e) {
        var color = e.target.value;
        applyAccent(color);
        localStorage.setItem('kiko-accent', color);
        updateColorDots(color);
    });

    initTheme();

    // --- Sidebar tabs ---
    var tabs = document.querySelectorAll('.sidebar-tab');
    var panels = document.querySelectorAll('.sidebar-panel');
    var playlistRefreshBtn = document.getElementById('playlist-refresh');
    var danmakuRefreshBtn = document.getElementById('danmaku-refresh');

    tabs.forEach(function (tab) {
        tab.addEventListener('click', function () {
            tabs.forEach(function (t) { t.classList.remove('active'); });
            panels.forEach(function (p) { p.classList.remove('active'); });
            tab.classList.add('active');
            document.getElementById('panel-' + tab.dataset.tab).classList.add('active');
            playlistRefreshBtn.style.display = tab.dataset.tab === 'playlist' ? '' : 'none';
            danmakuRefreshBtn.style.display = tab.dataset.tab === 'danmaku' ? '' : 'none';
        });
    });

    // --- Player & Playlist ---
    var currentDanmuPool = null;
    var recentPlaysEl = document.getElementById('recent-plays');
    var recentGridEl = recentPlaysEl.querySelector('.recent-plays-grid');

    function hideRecent() {
        recentPlaysEl.classList.add('hidden');
    }

    var player = new KikoPlayer(document.getElementById('dplayer'));
    var tree = new PlaylistTree(document.getElementById('playlist-tree'), {
        onSelect: function (node) {
            hideRecent();
            player.switchTo(node);
            updateNowPlaying(node);
            if (node.danmuPool) {
                currentDanmuPool = node.danmuPool;
                loadDanmaku(node.danmuPool);
            } else {
                currentDanmuPool = null;
                clearDanmaku();
            }
        }
    });

    async function loadPlaylist() {
        try {
            var data = await KikoAPI.getPlaylist();
            tree.setData(data);
        } catch (err) {
            console.error('Failed to load playlist:', err);
        }
    }

    function updateNowPlaying(node) {
        var el = document.getElementById('now-playing');
        var animeEl = el.querySelector('.now-playing-anime');
        var titleEl = el.querySelector('.now-playing-title');
        var infoEl = el.querySelector('.now-playing-info');
        if (node.animeName) {
            animeEl.textContent = node.animeName;
            animeEl.style.display = '';
            titleEl.textContent = node.text;
            infoEl.classList.remove('single');
        } else {
            animeEl.style.display = 'none';
            titleEl.textContent = node.text;
            infoEl.classList.add('single');
        }
        el.classList.add('visible');
    }

    document.getElementById('playlist-refresh').addEventListener('click', loadPlaylist);

    // --- Danmaku panel ---
    var danmakuList = document.getElementById('danmaku-list');
    var danmakuTotal = document.getElementById('danmaku-total');
    var expandedSources = new Set();

    function clearDanmaku() {
        danmakuList.innerHTML = '<div class="danmaku-empty">未加载弹幕</div>';
        danmakuTotal.textContent = '';
    }

    function formatTime(seconds) {
        var s = Math.round(seconds);
        var m = Math.floor(s / 60);
        s = s % 60;
        return m + ':' + (s < 10 ? '0' : '') + s;
    }

    function formatDelay(ms) {
        if (ms === 0) return '';
        var sign = ms > 0 ? '+' : '';
        return sign + (ms / 1000).toFixed(1) + 's';
    }

    async function loadDanmaku(poolId) {
        danmakuList.innerHTML = '<div class="danmaku-empty">加载中...</div>';
        danmakuTotal.textContent = '';
        try {
            var data = await KikoAPI.getDanmakuFull(poolId, false);
            renderDanmaku(data);
        } catch (err) {
            danmakuList.innerHTML = '<div class="danmaku-empty">加载失败</div>';
            console.error('Failed to load danmaku:', err);
        }
    }

    function fillComments(container, srcComments) {
        var display = srcComments.slice(0, 500);
        var fragment = document.createDocumentFragment();
        for (var i = 0; i < display.length; i++) {
            var c = display[i];
            var el = document.createElement('div');
            el.className = 'danmaku-comment';

            var time = document.createElement('span');
            time.className = 'danmaku-comment-time';
            time.textContent = formatTime(c[0]);
            el.appendChild(time);

            var text = document.createElement('span');
            text.className = 'danmaku-comment-text';
            text.textContent = c[4];
            el.appendChild(text);

            fragment.appendChild(el);
        }
        if (srcComments.length > 500) {
            var more = document.createElement('div');
            more.className = 'danmaku-comment';
            more.style.color = 'var(--text-muted)';
            more.textContent = '... 还有 ' + (srcComments.length - 500) + ' 条';
            fragment.appendChild(more);
        }
        container.appendChild(fragment);
        container._commentData = null;
    }

    function renderDanmaku(data) {
        var sources = data.source || [];
        var comments = data.comment || [];

        danmakuTotal.textContent = comments.length + ' 条弹幕';

        if (sources.length === 0 && comments.length === 0) {
            danmakuList.innerHTML = '<div class="danmaku-empty">无弹幕</div>';
            return;
        }

        // Group comments by source id
        var commentsBySource = {};
        comments.forEach(function (c) {
            // CommentF: [time, type, color, src, text, sender]
            var srcId = c[3];
            if (!commentsBySource[srcId]) commentsBySource[srcId] = [];
            commentsBySource[srcId].push(c);
        });

        // Sort comments within each source by time
        Object.keys(commentsBySource).forEach(function (srcId) {
            commentsBySource[srcId].sort(function (a, b) { return a[0] - b[0]; });
        });

        var fragment = document.createDocumentFragment();

        sources.forEach(function (src) {
            var srcComments = commentsBySource[src.id] || [];
            var isExpanded = expandedSources.has(src.id);

            var wrapper = document.createElement('div');
            wrapper.className = 'danmaku-source';

            // Source header row
            var row = document.createElement('div');
            row.className = 'danmaku-source-row';
            row.dataset.sourceId = src.id;

            var chevron = document.createElement('span');
            chevron.className = 'danmaku-source-chevron' + (isExpanded ? ' expanded' : '');
            row.appendChild(chevron);

            var name = document.createElement('span');
            name.className = 'danmaku-source-name';
            name.textContent = src.name || ('Source ' + src.id);
            row.appendChild(name);

            var delayStr = formatDelay(src.delay || 0);
            if (delayStr) {
                var delay = document.createElement('span');
                delay.className = 'danmaku-source-delay';
                delay.textContent = delayStr;
                row.appendChild(delay);
            }

            var count = document.createElement('span');
            count.className = 'danmaku-source-count';
            count.textContent = srcComments.length;
            row.appendChild(count);

            wrapper.appendChild(row);

            // Comments container — lazy: only populate when expanded
            var commentsWrap = document.createElement('div');
            commentsWrap.className = 'danmaku-comments collapsed';
            commentsWrap._commentData = srcComments;

            if (isExpanded) {
                fillComments(commentsWrap, srcComments);
                commentsWrap.classList.remove('collapsed');
            }

            wrapper.appendChild(commentsWrap);
            fragment.appendChild(wrapper);
        });

        danmakuList.innerHTML = '';
        danmakuList.appendChild(fragment);
    }

    // Danmaku list click delegation
    danmakuList.addEventListener('click', function (e) {
        var row = e.target.closest('.danmaku-source-row');
        if (!row) return;
        var sourceId = parseInt(row.dataset.sourceId);
        var chevron = row.querySelector('.danmaku-source-chevron');
        var commentsWrap = row.parentElement.querySelector('.danmaku-comments');

        if (expandedSources.has(sourceId)) {
            expandedSources.delete(sourceId);
            chevron.classList.remove('expanded');
            commentsWrap.classList.add('collapsed');
        } else {
            expandedSources.add(sourceId);
            chevron.classList.add('expanded');
            // Lazy render comments on first expand
            if (commentsWrap._commentData) {
                fillComments(commentsWrap, commentsWrap._commentData);
            }
            commentsWrap.classList.remove('collapsed');
        }
    });

    document.getElementById('danmaku-refresh').addEventListener('click', function () {
        if (currentDanmuPool) loadDanmaku(currentDanmuPool);
    });

    clearDanmaku();

    // --- Recent plays ---
    async function loadRecent() {
        try {
            var items = await KikoAPI.getRecent();
            renderRecent(items);
        } catch (err) {
            recentGridEl.innerHTML = '<div class="recent-plays-empty">加载失败</div>';
        }
    }

    function renderRecent(items) {
        recentGridEl.innerHTML = '';
        if (!items || items.length === 0) {
            recentGridEl.innerHTML = '<div class="recent-plays-empty">暂无最近播放</div>';
            return;
        }
        var fragment = document.createDocumentFragment();
        for (var i = 0; i < items.length; i++) {
            var item = items[i];
            var card = document.createElement('div');
            card.className = 'recent-card state-' + (item.playTimeState || 0);
            card._recentData = item;

            if (item.cover) {
                var img = document.createElement('img');
                img.className = 'recent-card-cover';
                img.src = 'data:image/jpeg;base64,' + item.cover;
                img.alt = '';
                img.loading = 'lazy';
                card.appendChild(img);
            } else {
                var ph = document.createElement('div');
                ph.className = 'recent-card-placeholder';
                ph.innerHTML = '<svg viewBox="0 0 24 24"><rect x="2" y="4" width="20" height="16" rx="2"/><path d="M10 9l5 3-5 3V9z"/></svg>';
                card.appendChild(ph);
            }

            var info = document.createElement('div');
            info.className = 'recent-card-info';
            if (item.animeName) {
                var anime = document.createElement('div');
                anime.className = 'recent-card-anime';
                anime.textContent = item.animeName;
                info.appendChild(anime);
            }
            var title = document.createElement('div');
            title.className = 'recent-card-title';
            title.textContent = item.text;
            info.appendChild(title);
            card.appendChild(info);

            var state = document.createElement('span');
            state.className = 'recent-card-state';
            card.appendChild(state);

            fragment.appendChild(card);
        }
        recentGridEl.appendChild(fragment);
    }

    recentGridEl.addEventListener('click', function (e) {
        var card = e.target.closest('.recent-card');
        if (!card || !card._recentData) return;
        var item = card._recentData;

        hideRecent();
        player.switchTo(item);
        updateNowPlaying(item);
        if (item.danmuPool) {
            currentDanmuPool = item.danmuPool;
            loadDanmaku(item.danmuPool);
        } else {
            currentDanmuPool = null;
            clearDanmaku();
        }
        tree.revealAndSelect(item.mediaId);
    });

    // --- Unload ---
    window.addEventListener('beforeunload', function () {
        player.destroy();
    });

    loadPlaylist();
    loadRecent();
})();
