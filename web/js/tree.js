class PlaylistTree {
    constructor(container, options) {
        this.container = container;
        this.onSelect = options.onSelect;
        this.data = [];
        this.activeMediaId = null;
        this.expandedPaths = new Set();

        this.container.addEventListener('click', (e) => this._handleClick(e));
    }

    setData(data) {
        this.data = data;
        this.render();
    }

    setActive(mediaId) {
        var old = this.container.querySelector('.tree-node--media.active');
        if (old) old.classList.remove('active');
        this.activeMediaId = mediaId;
        if (mediaId) {
            var el = this.container.querySelector('.tree-node--media[data-media-id="' + CSS.escape(mediaId) + '"]');
            if (el) el.classList.add('active');
        }
    }

    render() {
        this.container.innerHTML = '';
        if (!this.data || this.data.length === 0) {
            this.container.innerHTML = '<div class="playlist-empty">暂无内容</div>';
            return;
        }
        var fragment = document.createDocumentFragment();
        this._renderNodes(this.data, fragment, 0, '');
        this.container.appendChild(fragment);
    }

    _renderNodes(nodes, parent, depth, pathPrefix) {
        for (var i = 0; i < nodes.length; i++) {
            var node = nodes[i];
            var path = pathPrefix + '/' + i;
            var paddingLeft = depth * 20 + 12;

            if (node.nodes) {
                var folder = document.createElement('div');
                folder.className = 'tree-folder';

                var row = document.createElement('div');
                row.className = 'tree-node tree-node--folder';
                row.dataset.path = path;
                row.style.paddingLeft = paddingLeft + 'px';

                var chevron = document.createElement('span');
                var isExpanded = this.expandedPaths.has(path);
                chevron.className = 'tree-chevron' + (isExpanded ? ' expanded' : '');
                row.appendChild(chevron);

                if (node.marker !== undefined && node.marker !== null) {
                    var marker = document.createElement('span');
                    marker.className = 'tree-marker';
                    marker.dataset.marker = node.marker;
                    row.appendChild(marker);
                }

                var text = document.createElement('span');
                text.className = 'tree-text';
                text.textContent = node.text;
                row.appendChild(text);

                var count = document.createElement('span');
                count.className = 'tree-count';
                count.textContent = this._countMedia(node.nodes);
                row.appendChild(count);

                folder.appendChild(row);

                var childWrap = document.createElement('div');
                childWrap.className = 'tree-children collapsed';

                if (isExpanded) {
                    // Render children immediately if expanded
                    this._renderNodes(node.nodes, childWrap, depth + 1, path);
                    childWrap.classList.remove('collapsed');
                }
                // Store child data for lazy rendering
                childWrap._childData = node.nodes;
                childWrap._depth = depth + 1;
                childWrap._pathPrefix = path;

                folder.appendChild(childWrap);
                parent.appendChild(folder);
            } else {
                var el = document.createElement('div');
                var isActive = node.mediaId === this.activeMediaId;
                el.className = 'tree-node tree-node--media'
                    + (isActive ? ' active' : '')
                    + ' state-' + (node.playTimeState || 0);
                el.dataset.mediaId = node.mediaId;
                el.dataset.path = path;
                el.style.paddingLeft = paddingLeft + 'px';

                var stateIcon = document.createElement('span');
                stateIcon.className = 'tree-state-icon';
                el.appendChild(stateIcon);

                if (node.marker !== undefined && node.marker !== null) {
                    var marker2 = document.createElement('span');
                    marker2.className = 'tree-marker';
                    marker2.dataset.marker = node.marker;
                    el.appendChild(marker2);
                }

                var text2 = document.createElement('span');
                text2.className = 'tree-text';
                text2.textContent = node.text;
                if (node.color) {
                    text2.style.color = '#' + node.color;
                }
                el.appendChild(text2);

                el._nodeData = node;
                parent.appendChild(el);
            }
        }
    }

    _countMedia(nodes) {
        var count = 0;
        for (var i = 0; i < nodes.length; i++) {
            if (nodes[i].nodes) {
                count += this._countMedia(nodes[i].nodes);
            } else {
                count++;
            }
        }
        return count;
    }

    revealAndSelect(mediaId) {
        var path = this._findNodePath(this.data, mediaId, '');
        if (!path) {
            this.setActive(mediaId);
            return false;
        }
        var parts = path.split('/').filter(Boolean);
        var prefix = '';
        for (var i = 0; i < parts.length - 1; i++) {
            prefix += '/' + parts[i];
            this.expandedPaths.add(prefix);
        }
        this.render();
        this.setActive(mediaId);
        return true;
    }

    _findNodePath(nodes, mediaId, prefix) {
        for (var i = 0; i < nodes.length; i++) {
            var p = prefix + '/' + i;
            if (nodes[i].mediaId === mediaId) return p;
            if (nodes[i].nodes) {
                var found = this._findNodePath(nodes[i].nodes, mediaId, p);
                if (found) return found;
            }
        }
        return null;
    }

    _handleClick(e) {
        var folderNode = e.target.closest('.tree-node--folder');
        if (folderNode) {
            var path = folderNode.dataset.path;
            var chevron = folderNode.querySelector('.tree-chevron');
            var children = folderNode.parentElement.querySelector('.tree-children');

            if (this.expandedPaths.has(path)) {
                this.expandedPaths.delete(path);
                chevron.classList.remove('expanded');
                children.classList.add('collapsed');
            } else {
                this.expandedPaths.add(path);
                chevron.classList.add('expanded');
                // Lazy render: populate children on first expand
                if (children._childData) {
                    this._renderNodes(children._childData, children, children._depth, children._pathPrefix);
                    children._childData = null;
                }
                children.classList.remove('collapsed');
            }
            return;
        }

        var mediaNode = e.target.closest('.tree-node--media');
        if (mediaNode && mediaNode._nodeData) {
            this.setActive(mediaNode._nodeData.mediaId);
            this.onSelect(mediaNode._nodeData);
        }
    }
}
