class KikoPlayer {
    constructor(container) {
        this.dp = new DPlayer({
            container: container,
            mutex: true,
            theme: '#4fc3f7',
            danmaku: {
                api: KikoAPI.getDanmakuApi(),
                bottom: '15%'
            }
        });
        this.currentNode = null;
        this._progressInterval = null;

        this.dp.on('play', () => this._startProgressSync());
        this.dp.on('pause', () => this._stopProgressSync());
        this.dp.on('ended', () => this._onEnded());

        this._initDanmakuSpeedControl();
    }

    async switchTo(node) {
        if (this.currentNode) {
            this._postProgress();
        }
        this._stopProgressSync();
        this.currentNode = node;

        this.dp.switchVideo(
            { url: KikoAPI.getMediaUrl(node.mediaId) },
            { id: node.danmuPool, api: KikoAPI.getDanmakuApi(), bottom: '15%' }
        );

        // Check and load subtitle
        try {
            var subInfo = await KikoAPI.getSubtitle(node.mediaId);
            if (subInfo.type) {
                this._loadSubtitle(subInfo.type, node.mediaId);
            } else {
                this._clearSubtitle();
            }
        } catch (e) {
            // Subtitle check failed, continue without subtitle
        }

        if (node.playTimeState === 1 && node.playTime > 0) {
            this.dp.seek(node.playTime);
        }
        this.dp.play();
    }

    _loadSubtitle(format, mediaId) {
        var video = this.dp.video;
        // Remove existing track elements
        var tracks = video.querySelectorAll('track');
        tracks.forEach(function(t) { t.remove(); });

        var track = document.createElement('track');
        track.kind = 'subtitles';
        track.label = 'Subtitle';
        track.srclang = 'zh';
        track.src = KikoAPI.getSubtitleUrl(format, mediaId);
        track.default = true;
        video.appendChild(track);

        // Activate the track
        if (video.textTracks && video.textTracks.length > 0) {
            video.textTracks[video.textTracks.length - 1].mode = 'showing';
        }
    }

    _clearSubtitle() {
        var video = this.dp.video;
        var tracks = video.querySelectorAll('track');
        tracks.forEach(function(t) { t.remove(); });
    }

    _computeState() {
        var curTime = this.dp.video.currentTime;
        var duration = this.dp.video.duration;
        if (!duration || isNaN(duration)) return 0;
        if (curTime > 15 && curTime < duration - 15) return 1;
        if (curTime >= duration - 15 && duration > 15) return 2;
        return 0;
    }

    _postProgress() {
        if (!this.currentNode) return;
        var state = this._computeState();
        if (state !== 0) {
            KikoAPI.updateTime(
                this.currentNode.mediaId,
                this.dp.video.currentTime,
                state
            );
        }
    }

    _postProgressSync() {
        if (!this.currentNode) return;
        var state = this._computeState();
        if (state !== 0) {
            KikoAPI.updateTimeSync(
                this.currentNode.mediaId,
                this.dp.video.currentTime,
                state
            );
        }
    }

    _startProgressSync() {
        this._stopProgressSync();
        this._progressInterval = setInterval(() => this._postProgress(), 15000);
    }

    _stopProgressSync() {
        if (this._progressInterval) {
            clearInterval(this._progressInterval);
            this._progressInterval = null;
        }
    }

    _onEnded() {
        this._stopProgressSync();
        if (this.currentNode) {
            KikoAPI.updateTime(this.currentNode.mediaId, 0, 2);
        }
    }

    destroy() {
        this._stopProgressSync();
        this._postProgressSync();
    }

    _applyDanmakuSpeed(speed) {
        var m = parseFloat(speed);
        document.documentElement.style.setProperty('--danmaku-roll-duration', (10 / m).toFixed(2) + 's');
        document.documentElement.style.setProperty('--danmaku-fixed-duration', (6 / m).toFixed(2) + 's');
    }

    _initDanmakuSpeedControl() {
        var self = this;
        var isMobile = window.innerWidth < 768;
        var saved = localStorage.getItem('kiko-danmaku-speed') || (isMobile ? '1.5' : '1');
        var speeds = ['0.5', '0.75', '1', '1.5', '2'];

        // Build DOM
        var wrapper = document.createElement('div');
        wrapper.className = 'kiko-danmaku-speed';

        var btn = document.createElement('button');
        btn.className = 'dplayer-icon kiko-danmaku-speed-btn';
        btn.setAttribute('data-balloon', '弹幕速度');
        btn.setAttribute('data-balloon-pos', 'up');

        var label = document.createElement('span');
        label.className = 'kiko-danmaku-speed-label';
        label.textContent = saved + 'x';
        btn.appendChild(label);
        wrapper.appendChild(btn);

        var panel = document.createElement('div');
        panel.className = 'kiko-danmaku-speed-panel';

        var title = document.createElement('div');
        title.className = 'kiko-danmaku-speed-title';
        title.textContent = '弹幕速度';
        panel.appendChild(title);

        for (var i = 0; i < speeds.length; i++) {
            var item = document.createElement('div');
            item.className = 'kiko-danmaku-speed-item' + (speeds[i] === saved ? ' active' : '');
            item.dataset.speed = speeds[i];
            item.textContent = speeds[i] + 'x';
            panel.appendChild(item);
        }
        wrapper.appendChild(panel);

        // Inject into control bar
        var iconsRight = this.dp.container.querySelector('.dplayer-icons-right');
        var fullBtn = iconsRight.querySelector('.dplayer-full');
        iconsRight.insertBefore(wrapper, fullBtn);

        // Toggle panel
        btn.addEventListener('click', function (e) {
            e.stopPropagation();
            panel.classList.toggle('kiko-panel-open');
        });

        // Select speed
        panel.addEventListener('click', function (e) {
            var target = e.target.closest('.kiko-danmaku-speed-item');
            if (!target) return;
            var speed = target.dataset.speed;
            panel.querySelectorAll('.kiko-danmaku-speed-item').forEach(function (el) {
                el.classList.toggle('active', el === target);
            });
            label.textContent = speed + 'x';
            self._applyDanmakuSpeed(speed);
            localStorage.setItem('kiko-danmaku-speed', speed);
            panel.classList.remove('kiko-panel-open');
        });

        // Close on outside click
        document.addEventListener('click', function (e) {
            if (!e.target.closest('.kiko-danmaku-speed')) {
                panel.classList.remove('kiko-panel-open');
            }
        });

        // Apply saved speed
        this._applyDanmakuSpeed(saved);
    }
}
