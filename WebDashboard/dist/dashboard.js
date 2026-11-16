import { createDailyChart } from './daily-chart.js';
import { reveal, revealRows, updateList } from './dashboard-motion.js';

initializeTheme();

const activityPanel = document.getElementById('activity');
const placeholder = document.getElementById('placeholder');
const connection = document.getElementById('connection');
const dataSourceLabel = document.getElementById('data-source-label');
const localProjectDetails = document.getElementById('local-project');
const projectPathLabel = document.getElementById('project-path-label');
const projectPath = document.getElementById('project-path');
const editorConnection = document.getElementById('editor-connection');
const assetName = document.getElementById('asset-name');
const packagePath = document.getElementById('package-path');
const activityType = document.getElementById('activity-type');
const alias = document.getElementById('alias');
const timestamp = document.getElementById('timestamp');
const eventId = document.getElementById('event-id');
const latestSaves = document.getElementById('latest-saves');
const latestHistoryLink = document.getElementById('latest-history-link');
const latestAssetReferences = document.getElementById('latest-asset-references');
const historyTable = document.getElementById('history');
const historyRows = document.getElementById('history-rows');
const historyPlaceholder = document.getElementById('history-placeholder');
const editsTable = document.getElementById('edits');
const editRows = document.getElementById('edits-rows');
const editsPlaceholder = document.getElementById('edits-placeholder');
const dailyChart = document.getElementById('daily-chart');
const dailyRows = document.getElementById('daily-rows');
const dailyPlaceholder = document.getElementById('daily-placeholder');
const chartDateSpan = document.getElementById('chart-date-span');
const totalBuckets = document.getElementById('total-buckets');
const totalSaves = document.getElementById('total-saves');
const totalAssets = document.getElementById('total-assets');
const topAssetFolders = document.getElementById('top-asset-folders');
const topAssetsPlaceholder = document.getElementById('top-assets-placeholder');
const assetFolderTemplate = document.getElementById('asset-folder-template');
const assetFoldersByPath = new Map();
const sharedAssetsList = {
  table: document.getElementById('shared-assets'),
  rows: document.getElementById('shared-assets-rows'),
  placeholder: document.getElementById('shared-assets-placeholder'),
  countKeys: ['aliasCount', 'rawSaveCount'],
};
const hotspotList = {
  table: document.getElementById('hotspot-assets'),
  rows: document.getElementById('hotspot-rows'),
  placeholder: document.getElementById('hotspot-placeholder'),
  countKeys: ['activityBucketCount', 'aliasCount', 'referencerCount', 'rawSaveCount'],
};
const hotspotRules = document.getElementById('hotspot-rules');
const hotspotReferences = document.getElementById('hotspot-references');
const assetLists = [sharedAssetsList, hotspotList];
const rangeControls = document.getElementById('range-days');
const topAssetsRange = document.getElementById('top-assets-range');
const assetDetail = document.getElementById('asset-detail');
const assetDetailSlots = new Map([
  ['/', document.getElementById('overview-detail-slot')],
  ['/assets', document.getElementById('assets-detail-slot')],
  ['/collaboration', document.getElementById('collaboration-detail-slot')],
  ['/history', document.getElementById('history-detail-slot')],
  ['/edits', document.getElementById('edits-detail-slot')],
]);
const assetDetailPath = document.getElementById('asset-detail-path');
const assetDetailStatus = document.getElementById('asset-detail-status');
const assetReferenceDetail = document.getElementById('asset-reference-detail');
const assetReferenceStatus = document.getElementById('asset-reference-status');
const assetReferenceGraph = document.getElementById('asset-reference-graph');
const assetReferenceSource = document.getElementById('asset-reference-source');
const assetReferencePaths = document.getElementById('asset-reference-paths');
const assetOpenHint = document.getElementById('asset-open-hint');
const assetOpenStatus = document.getElementById('asset-open-status');
const assetAliasTable = document.getElementById('asset-alias-table');
const assetAliasRows = document.getElementById('asset-alias-rows');
const assetAliasCaption = document.getElementById('asset-alias-caption');
const assetDetailTable = document.getElementById('asset-detail-table');
const assetDetailRows = document.getElementById('asset-detail-rows');
const assetHistoryCaption = document.getElementById('asset-history-caption');
const closeAssetDetail = document.getElementById('close-asset-detail');
const sectionLinks = [...document.querySelectorAll('.sidebar-nav a')];
const pageTitle = document.getElementById('page-title');
const pages = new Map([
  ['/', document.getElementById('overview-page')],
  ['/assets', document.getElementById('assets-page')],
  ['/collaboration', document.getElementById('collaboration-page')],
  ['/history', document.getElementById('history-page')],
  ['/edits', document.getElementById('edits-page')],
]);
const oldPageLinks = new Map([
  ['#overview', '/'], ['#assets', '/assets'], ['#collaboration', '/collaboration'], ['#recent', '/history'],
]);

const refreshIntervalMs = 2000;
const requestTimeoutMs = 5000;
const openAssetTimeoutMs = 11000;
const assetsPerFolderPage = 20;
const numberFormat = new Intl.NumberFormat('en-US');
const dailyGraph = createDailyChart(numberFormat);
const utcFormat = new Intl.DateTimeFormat('en-US', {
  dateStyle: 'medium',
  timeStyle: 'medium',
  timeZone: 'UTC',
});
const shortUtcFormat = new Intl.DateTimeFormat('en-US', {
  month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit', hourCycle: 'h23', timeZone: 'UTC',
});
const dayFormat = new Intl.DateTimeFormat('en-US', { month: 'short', day: 'numeric', timeZone: 'UTC' });
const noSaves = 'No saves yet.';
const failedActivity = 'Could not load activity.';

let activePage;
let displayedEventId;
let displayedHistory;
let displayedEdits;
let displayedDaily;
let renderedRange;
let activeRequest;
let refreshTimer;
let selectedAsset = null;
let assetRequest;
let displayedAssetHistory;
let displayedAssetAliasCounts;
let displayedAssetReferences;

function initializeTheme() {
  const button = document.getElementById('theme-toggle');
  const systemTheme = matchMedia('(prefers-color-scheme: dark)');
  const storageKey = 'pav-theme';
  let preference;
  try {
    const savedTheme = localStorage.getItem(storageKey);
    if (savedTheme === 'light' || savedTheme === 'dark') preference = savedTheme;
  } catch {
    // The dashboard still follows the system preference when storage is blocked.
  }

  function applyTheme() {
    const theme = preference ?? (systemTheme.matches ? 'dark' : 'light');
    document.documentElement.dataset.theme = theme;
    button.setAttribute('aria-pressed', String(theme === 'dark'));
    button.title = theme === 'dark' ? 'Switch to light mode' : 'Switch to dark mode';
  }

  button.addEventListener('click', () => {
    preference = document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark';
    applyTheme();
    try {
      localStorage.setItem(storageKey, preference);
    } catch {
      // The selection remains active for the current page.
    }
  });

  systemTheme.addEventListener('change', applyTheme);
  applyTheme();
  button.hidden = false;
}

function showConnection(indicator, state, message) {
  if (indicator.dataset.state !== state) {
    indicator.dataset.state = state;
    indicator.textContent = message;
  }
}

function showLocalProject(project) {
  localProjectDetails.hidden = !project;
  if (!project) return;

  const label = project.projectPath ? 'Project' : 'Activity database';
  const path = project.projectPath ?? project.databasePath;
  if (projectPathLabel.textContent !== label) projectPathLabel.textContent = label;
  if (projectPath.textContent !== path) projectPath.textContent = path;
  if (project.editorConnected === true) {
    showConnection(editorConnection, 'connected', 'Unreal Editor · Connected');
  } else if (project.editorConnected === false) {
    showConnection(editorConnection, 'offline', 'Unreal Editor · Offline');
  } else {
    showConnection(editorConnection, 'unknown', 'Unreal Editor · Unknown');
  }
}

function showActivity(activity) {
  if (activity === null) {
    activityPanel.hidden = true;
    placeholder.hidden = false;
    placeholder.textContent = noSaves;
    displayedEventId = undefined;
    return;
  }

  if (activity.eventId === displayedEventId) return;

  assetName.textContent = activity.packagePath.split('/').pop();
  packagePath.textContent = activity.packagePath;
  latestAssetReferences.dataset.packagePath = activity.packagePath;
  activityType.textContent = activity.activityType === 0 ? 'Asset saved' : 'Activity';
  alias.textContent = activity.contributorAlias || 'Unspecified';
  showEventTime(timestamp, activity.timestampUtc);
  eventId.textContent = activity.eventId;

  placeholder.hidden = true;
  activityPanel.hidden = false;
  reveal(activityPanel.querySelector('.asset'), { distance: 3, duration: 215 });
  displayedEventId = activity.eventId;
}

function showEventTime(element, timestampUtc, format = utcFormat) {
  const eventTime = new Date(timestampUtc);
  element.dateTime = timestampUtc;
  element.textContent = Number.isNaN(eventTime.getTime())
    ? timestampUtc
    : `${format.format(eventTime)} UTC`;
}

function addAssetCell(row, path) {
  const assetCell = row.insertCell();
  assetCell.className = 'asset-cell';
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'text-button';
  button.textContent = path.split('/').pop();
  button.dataset.packagePath = path;
  button.setAttribute('aria-label', `View details for ${path}`);
  button.setAttribute('aria-controls', 'asset-detail');
  button.addEventListener('click', () => selectAsset(path, button));
  const packageLabel = document.createElement('p');
  packageLabel.className = 'package-path';
  packageLabel.textContent = path;
  assetCell.append(button, packageLabel);
}

function historyRow(activity) {
  const row = document.createElement('tr');
  addAssetCell(row, activity.packagePath);
  row.insertCell().textContent = activity.contributorAlias || 'Unspecified';
  const time = document.createElement('time');
  showEventTime(time, activity.timestampUtc);
  row.insertCell().append(time);
  return row;
}

function latestSaveRow(activity) {
  const item = document.createElement('li');
  const info = document.createElement('div');
  const name = document.createElement('strong');
  name.textContent = activity.packagePath.split('/').pop();
  name.title = activity.packagePath;
  const details = document.createElement('span');
  details.textContent = activity.contributorAlias || 'Unspecified';
  info.append(name, details);
  const time = document.createElement('time');
  showEventTime(time, activity.timestampUtc, shortUtcFormat);
  item.append(info, time);
  return item;
}

function showHistory(activities) {
  const historyData = JSON.stringify(activities);
  if (historyData === displayedHistory) return;

  historyTable.hidden = activities.length === 0;
  historyPlaceholder.hidden = !historyTable.hidden;
  historyPlaceholder.textContent = noSaves;
  const focusedPackage = historyRows.contains(document.activeElement) ? document.activeElement.dataset.packagePath : null;
  updateList(historyRows, activities, activity => activity.eventId, historyRow);
  if (focusedPackage && !historyRows.contains(document.activeElement)) {
    restoreAssetFocus(focusedPackage, historyRows, { preventScroll: true });
  }

  const priorSaves = activities.slice(1, 4);
  latestSaves.hidden = priorSaves.length === 0;
  latestHistoryLink.hidden = activities.length === 0;
  updateList(latestSaves, priorSaves, activity => activity.eventId, latestSaveRow);
  displayedHistory = historyData;
}

function showEditHistory({ status, edits }) {
  const data = JSON.stringify([status, edits]);
  if (data === displayedEdits) return;

  editsTable.hidden = edits.length === 0;
  editsPlaceholder.hidden = !editsTable.hidden;
  const messages = {
    ready: 'No edits recorded yet.',
    unavailable: 'The project database has no edit history table. Open the project with a plugin build that records edits.',
    team: 'Edit history is available from the local Unreal project database.',
  };
  editsPlaceholder.textContent = messages[status];
  const focusedPackage = editRows.contains(document.activeElement) ? document.activeElement.dataset.packagePath : null;
  updateList(editRows, edits, edit => edit.eventId, historyRow);
  if (focusedPackage && !editRows.contains(document.activeElement)) {
    restoreAssetFocus(focusedPackage, editRows, { preventScroll: true });
  }
  displayedEdits = data;
}

function showOverviewTotals(daily, folders) {
  const buckets = daily.reduce((total, day) => total + day.activityBucketCount, 0);
  const saves = daily.reduce((total, day) => total + day.rawSaveCount, 0);
  const assets = folders.reduce((total, folder) => total + folder.assets.length, 0);
  for (const [element, value] of [[totalBuckets, buckets], [totalSaves, saves], [totalAssets, assets]]) {
    const formatted = numberFormat.format(value);
    if (element.textContent !== formatted) {
      element.textContent = formatted;
      reveal(element, { distance: 3, duration: 190 });
    }
  }
}

function showDaily(daily, rangeChanged) {
  const dailyData = JSON.stringify(daily);
  if (dailyData === displayedDaily) return;

  const rows = document.createDocumentFragment();

  for (const day of daily) {
    const row = document.createElement('tr');
    const dateCell = document.createElement('th');
    dateCell.scope = 'row';
    const date = document.createElement('time');
    date.dateTime = day.dateUtc;
    date.textContent = day.dateUtc;
    dateCell.append(date);
    row.append(dateCell);

    row.insertCell().textContent = numberFormat.format(day.activityBucketCount);
    row.insertCell().textContent = numberFormat.format(day.rawSaveCount);
    rows.append(row);
  }

  dailyRows.replaceChildren(rows);
  dailyChart.hidden = false;
  dailyPlaceholder.hidden = true;
  const dateSpan = `${dayFormat.formatRange(new Date(daily[0].dateUtc), new Date(daily.at(-1).dateUtc))} · UTC`;
  if (chartDateSpan.textContent !== dateSpan) chartDateSpan.textContent = dateSpan;
  dailyGraph.setData(daily, rangeChanged);
  displayedDaily = dailyData;
}

function renderAssetRows(list, assets) {
  const data = JSON.stringify(assets);
  if (data === list.displayedData) return;

  const focusedAsset = list.rows.contains(document.activeElement)
    ? document.activeElement.dataset.packagePath
    : undefined;
  updateList(list.rows, assets, asset => asset.packagePath, asset => {
    const row = document.createElement('tr');
    addAssetCell(row, asset.packagePath);
    for (const key of list.countKeys) {
      const count = asset[key];
      row.insertCell().textContent = count == null ? '—' : numberFormat.format(count);
    }
    return row;
  });
  list.displayedData = data;
  if (focusedAsset && !list.rows.contains(document.activeElement)) {
    restoreAssetFocus(focusedAsset, list.rows, { preventScroll: true });
  }
}

function showAssetList(list, assets, emptyMessage) {
  list.table.hidden = assets.length === 0;
  list.placeholder.hidden = !list.table.hidden;
  list.placeholder.textContent = emptyMessage;
  renderAssetRows(list, assets);
}

function renderOpenFolder(folder) {
  if (!folder.details.open) return;

  renderAssetRows(folder, folder.assets.slice(0, folder.visibleCount));
  const remaining = folder.assets.length - folder.visibleCount;
  folder.more.hidden = remaining <= 0;
  if (!folder.more.hidden) {
    const moreLabel = `Show ${Math.min(assetsPerFolderPage, remaining)} more`;
    if (folder.more.textContent !== moreLabel) folder.more.textContent = moreLabel;
  }
}

function createAssetFolder(folderPath) {
  const details = assetFolderTemplate.content.firstElementChild.cloneNode(true);
  details.dataset.folderPath = folderPath;
  details.querySelector('.asset-folder-path').textContent = folderPath;
  details.querySelector('table').setAttribute('aria-label', `Assets in ${folderPath}`);
  const folder = {
    details,
    summary: details.querySelector('summary'),
    count: details.querySelector('.asset-folder-count'),
    rows: details.querySelector('tbody'),
    more: details.querySelector('.asset-folder-more'),
    countKeys: ['activityBucketCount', 'rawSaveCount'],
    visibleCount: assetsPerFolderPage,
    assets: [],
  };
  details.addEventListener('toggle', () => {
    if (!details.open) return;
    renderOpenFolder(folder);
    revealRows([...folder.rows.rows]);
  });
  folder.more.addEventListener('click', () => {
    const firstNewRow = folder.visibleCount;
    folder.visibleCount += assetsPerFolderPage;
    renderOpenFolder(folder);
    folder.rows.rows[firstNewRow]?.querySelector('button')?.focus();
  });
  return folder;
}

function showAssetFolders(folders, rangeDays) {
  topAssetFolders.hidden = folders.length === 0;
  topAssetsPlaceholder.hidden = !topAssetFolders.hidden;
  topAssetsPlaceholder.textContent = `No assets were saved in the last ${rangeDays} days.`;

  const activeElement = document.activeElement;
  const activeFolderPath = activeElement?.closest('#top-asset-folders > details')?.dataset.folderPath;
  const currentPaths = new Set();
  let nextFolder = topAssetFolders.firstElementChild;
  for (const { folderPath, assets } of folders) {
    currentPaths.add(folderPath);
    let folder = assetFoldersByPath.get(folderPath);
    const isNewFolder = !folder;
    if (!folder) {
      folder = createAssetFolder(folderPath);
      assetFoldersByPath.set(folderPath, folder);
    }
    folder.assets = assets;
    const countLabel = `${numberFormat.format(assets.length)} ${assets.length === 1 ? 'asset' : 'assets'}`;
    if (folder.details !== nextFolder) {
      if (!isNewFolder && topAssetFolders.moveBefore) topAssetFolders.moveBefore(folder.details, nextFolder);
      else topAssetFolders.insertBefore(folder.details, nextFolder);
    }
    nextFolder = folder.details.nextElementSibling;
    if (folder.count.textContent !== countLabel) {
      folder.count.textContent = countLabel;
      if (!isNewFolder) reveal(folder.count, { distance: 2, duration: 200 });
    }
    if (isNewFolder && currentPaths.size <= 8) reveal(folder.details, { delay: (currentPaths.size - 1) * 23 });
    renderOpenFolder(folder);
  }
  for (const [path, folder] of assetFoldersByPath) {
    if (!currentPaths.has(path)) {
      folder.details.remove();
      assetFoldersByPath.delete(path);
    }
  }
  if (activeFolderPath && document.activeElement === document.body) {
    const folder = assetFoldersByPath.get(activeFolderPath);
    const focusTarget = folder && activeElement.isConnected && !activeElement.hidden
      ? activeElement
      : folder?.summary ?? rangeControls.querySelector(':checked');
    focusTarget.focus({ preventScroll: true });
  }
}

function restoreAssetFocus(path, rows, options) {
  const details = rows.closest('.asset-folder');
  const folder = assetFoldersByPath.get(details?.dataset.folderPath);
  if (folder) rows = folder.rows;
  const buttons = rows.querySelectorAll('button[data-package-path]');
  const button = Array.from(buttons).find(item => item.dataset.packagePath === path);
  const pageControl = rangeControls.hidden ? pageTitle : rangeControls.querySelector(':checked');
  const fallback = folder && !topAssetFolders.hidden ? folder.summary : pageControl;
  (button && !button.closest('[hidden], details:not([open])') ? button : fallback).focus(options);
}

function showHotspotCriteria(hotspots) {
  const snapshot = hotspots.referenceSnapshot;
  const contributors = `${hotspots.minNamedAliases}+ named aliases`;
  const context = snapshot.status === 'team'
    ? contributors
    : `either ${contributors} or ${hotspots.minReferencers}+ direct /Game/ referencers`;
  const rule = `From the top ${hotspots.sampleLimit} assets by activity: ${hotspots.minActivityBuckets}+ buckets and ${context}.`;
  if (hotspotRules.textContent !== rule) hotspotRules.textContent = rule;
  hotspotRules.hidden = false;

  let status = 'Refresh references in Unreal to include direct referencers.';
  if (snapshot.status === 'team') {
    status = '';
  } else if (snapshot.status === 'ready') {
    const capturedAt = utcFormat.format(new Date(snapshot.capturedAtUtc));
    if (snapshot.automatic) {
      status = snapshot.stale
        ? `Waiting for reference refresh · Last updated ${capturedAt} UTC`
        : `References refreshed automatically · ${capturedAt} UTC`;
    } else {
      status = `References refreshed manually · ${capturedAt} UTC`;
    }
  } else if (snapshot.status === 'unavailable') {
    status = 'Could not read the reference snapshot. Waiting for the next refresh.';
  }
  if (hotspotReferences.textContent !== status) hotspotReferences.textContent = status;
  hotspotReferences.hidden = !status;
}

function cancelAssetRequest() {
  assetRequest?.abort();
  assetRequest = undefined;
}

function resetAssetHistory() {
  cancelAssetRequest();
  displayedAssetHistory = undefined;
  displayedAssetAliasCounts = undefined;
  displayedAssetReferences = undefined;
  assetOpenStatus.textContent = '';
  assetReferenceDetail.hidden = true;
  assetAliasTable.hidden = true;
  assetDetailTable.hidden = true;
  assetDetailStatus.hidden = false;
  assetDetailStatus.textContent = 'Loading timeline…';
}

function selectAsset(path, trigger) {
  selectedAsset = { path, trigger, rows: trigger.closest('tbody'), opening: false };
  assetDetailSlots.get(activePage).append(assetDetail);
  assetDetailPath.textContent = path;
  assetDetail.hidden = false;
  resetAssetHistory();
  closeAssetDetail.focus({ preventScroll: true });
  assetDetail.scrollIntoView({ block: 'nearest' });
  reveal(assetDetail);
  refreshAssetHistory();
}

function fillReferenceNode(node, path, canOpen) {
  const name = document.createElement('strong');
  name.textContent = path.split('/').pop();
  const packageLabel = document.createElement('span');
  packageLabel.className = 'package-path';
  packageLabel.textContent = path;
  node.dataset.packagePath = path;
  node.disabled = !canOpen;
  const label = canOpen ? `Open ${path} in Unreal Editor` : path;
  node.title = label;
  node.setAttribute('aria-label', label);
  node.replaceChildren(name, packageLabel);
}

function showAssetReferences(path, dependencies, snapshotStatus, canOpen) {
  if (snapshotStatus === 'team') {
    assetReferenceDetail.hidden = true;
    return;
  }
  const data = JSON.stringify([path, dependencies, snapshotStatus, canOpen]);
  if (data === displayedAssetReferences) return;

  const rows = document.createDocumentFragment();
  for (const target of dependencies ?? []) {
    const row = document.createElement('li');
    const node = document.createElement('button');
    node.type = 'button';
    node.className = 'reference-node';
    fillReferenceNode(node, target, canOpen);
    row.append(node);
    rows.append(row);
  }
  assetReferencePaths.replaceChildren(rows);
  const hasDependencies = Boolean(dependencies?.length);
  if (hasDependencies) fillReferenceNode(assetReferenceSource, path, canOpen);
  assetOpenHint.hidden = !hasDependencies || !canOpen;
  assetReferenceGraph.hidden = !hasDependencies;
  assetReferenceStatus.hidden = hasDependencies;
  const statusMessages = {
    missing: 'No reference snapshot found. Run the reference export script in Unreal.',
    unavailable: 'The reference snapshot could not be read. Run the reference export script again.',
    ready: 'This asset is missing from the dependency snapshot. Save it in Unreal, then refresh references.',
  };
  assetReferenceStatus.textContent = dependencies === null
    ? statusMessages[snapshotStatus] ?? 'Dependency information is unavailable. Refresh references in Unreal.'
    : 'This asset has no direct /Game/ dependencies.';
  assetReferenceDetail.hidden = false;
  if (hasDependencies) reveal(assetReferenceGraph);
  displayedAssetReferences = data;
}

async function openAssetInEditor(path) {
  const selection = selectedAsset;
  if (selection === null || selection.opening) return;
  selection.opening = true;

  const name = path.split('/').pop();
  assetOpenStatus.textContent = `Opening ${name} in Unreal Editor…`;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), openAssetTimeoutMs);
  let status = 'failed';
  try {
    const response = await fetch('/api/editor/open-asset', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ packagePath: path }),
      signal: controller.signal,
    });
    status = (await response.json()).status;
  } catch (error) {
    if (error.name === 'AbortError') status = 'timeout';
  } finally {
    clearTimeout(timeout);
    selection.opening = false;
  }

  if (selectedAsset !== selection) return;
  const messages = {
    opened: `Opened ${name} in Unreal Editor.`,
    offline: 'Open this project in Unreal and start the PAV reference script, then try again.',
    not_found: `Unreal could not find ${name} in this project.`,
    timeout: 'Unreal did not respond in time. Check the editor, then try again.',
    unavailable: 'Opening assets is available from the local project dashboard.',
    invalid: 'This node does not have a valid Unreal project asset path.',
  };
  assetOpenStatus.textContent = messages[status] ?? `Could not open ${name} in Unreal Editor.`;
}

function showAssetAliasCounts(aliasCounts) {
  assetAliasTable.hidden = aliasCounts.length === 0;
  const data = JSON.stringify(aliasCounts);
  if (data !== displayedAssetAliasCounts) {
    updateList(assetAliasRows, aliasCounts, alias => alias.contributorAlias, alias => {
      const row = document.createElement('tr');
      row.insertCell().textContent = alias.contributorAlias || 'Unspecified';
      row.insertCell().textContent = numberFormat.format(alias.saveCount);
      return row;
    });
    displayedAssetAliasCounts = data;
  }
}

function showAssetHistory(activities, editHistory) {
  const events = [
    ...activities.map(save => ({ ...save, kind: 'save' })),
    ...editHistory.edits.map(edit => ({ ...edit, kind: 'edit' })),
  ];
  // Both sources write UTC ISO timestamps with millisecond precision.
  events.sort((a, b) => b.timestampUtc.localeCompare(a.timestampUtc)
    || b.kind.localeCompare(a.kind) || b.eventId.localeCompare(a.eventId));
  assetDetailTable.hidden = events.length === 0;
  const data = JSON.stringify(events);
  if (data !== displayedAssetHistory) {
    updateList(assetDetailRows, events, event => `${event.kind}:${event.eventId}`, event => {
      const row = document.createElement('tr');
      const time = document.createElement('time');
      showEventTime(time, event.timestampUtc);
      row.insertCell().append(time);
      const badge = document.createElement('span');
      badge.className = 'timeline-event';
      badge.dataset.type = event.kind;
      badge.textContent = event.kind === 'edit' ? 'Edit detected' : 'Saved';
      row.insertCell().append(badge);
      row.insertCell().textContent = event.contributorAlias?.trim() || 'Unspecified';
      return row;
    });
    displayedAssetHistory = data;
  }
  assetDetailStatus.hidden = !assetDetailTable.hidden;
  assetDetailStatus.textContent = editHistory.status === 'ready' ? 'No edits or saves to display.' : 'No saves to display.';
}

async function refreshAssetHistory() {
  if (selectedAsset === null || assetRequest) return;

  const controller = new AbortController();
  assetRequest = controller;
  const { path } = selectedAsset;
  const rangeDays = rangeControls.hidden ? 'all' : selectedRangeDays();
  const query = new URLSearchParams({ package: path, days: String(rangeDays) });
  const timeout = setTimeout(() => controller.abort(), requestTimeoutMs);

  try {
    const response = await fetch(`/api/activity/asset?${query}`, {
      cache: 'no-store',
      signal: controller.signal,
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    if (assetRequest !== controller) return;
    if (data.packagePath !== path || data.rangeDays !== rangeDays
        || !Array.isArray(data.aliasCounts) || !Array.isArray(data.activities)) {
      throw new Error('Unexpected asset history response');
    }
    const editHistory = data.editHistory ?? { status: 'unavailable', edits: [] };
    const rangeLabel = rangeDays === 'all' ? 'All dates' : `Last ${rangeDays} days`;
    const aliasLabel = `Saves by alias · ${rangeLabel}`;
    const historyLabel = editHistory.status === 'ready'
      ? `Edit and save timeline · ${rangeLabel} · Up to 10 per event type`
      : `Recent saves · ${rangeLabel} · Up to 10`;
    if (assetAliasCaption.textContent !== aliasLabel) assetAliasCaption.textContent = aliasLabel;
    if (assetHistoryCaption.textContent !== historyLabel) assetHistoryCaption.textContent = historyLabel;
    showAssetReferences(path, data.dependencies ?? null,
      data.referenceSnapshot?.status ?? (data.references === null ? 'team' : undefined),
      data.canOpenInEditor === true);
    showAssetAliasCounts(data.aliasCounts);
    showAssetHistory(data.activities, editHistory);
  } catch {
    if (assetRequest !== controller) return;
    assetDetailStatus.hidden = false;
    assetDetailStatus.textContent = 'Could not load the timeline. Retrying…';
  } finally {
    clearTimeout(timeout);
    if (assetRequest === controller) assetRequest = undefined;
  }
}

function selectedRangeDays() {
  return Number(rangeControls.querySelector(':checked').value);
}

async function refreshActivity() {
  clearTimeout(refreshTimer);
  if (activeRequest) activeRequest.abort();

  const controller = new AbortController();
  activeRequest = controller;
  const rangeDays = selectedRangeDays();
  refreshAssetHistory();
  const timeout = setTimeout(() => controller.abort(), requestTimeoutMs);

  try {
    const response = await fetch(`/api/activity/dashboard?days=${rangeDays}&folders=1`, {
      cache: 'no-store',
      signal: controller.signal,
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const {
      activities, editHistory, daily, assetFolders, multiAliasAssets, hotspots,
      rangeDays: responseDays, demo, teamProject, localProject,
    } = await response.json();
    if (activeRequest !== controller) return;
    if (responseDays !== rangeDays) throw new Error('Unexpected dashboard period');
    const sourceLabel = teamProject ? `Team project · ${teamProject}` : demo === true ? 'Sample data' : '';
    if (dataSourceLabel.textContent !== sourceLabel) dataSourceLabel.textContent = sourceLabel;
    dataSourceLabel.hidden = !sourceLabel;
    document.documentElement.classList.toggle('team-mode', Boolean(teamProject));
    showLocalProject(localProject);

    const rangeLabel = `Last ${rangeDays} days · UTC`;
    if (topAssetsRange.textContent !== rangeLabel) topAssetsRange.textContent = rangeLabel;
    showActivity(activities[0] ?? null);
    showHistory(activities);
    showEditHistory(editHistory ?? { status: 'unavailable', edits: [] });
    showOverviewTotals(daily, assetFolders);
    showDaily(daily, renderedRange !== rangeDays);
    showAssetFolders(assetFolders, rangeDays);
    showAssetList(sharedAssetsList, multiAliasAssets, 'No assets were saved under multiple aliases in this date range.');
    showAssetList(hotspotList, hotspots.candidates, 'No assets meet the hotspot criteria in this date range.');
    showHotspotCriteria(hotspots);
    renderedRange = rangeDays;
    showConnection(connection, 'connected', 'Live');
  } catch {
    if (activeRequest !== controller) return;
    showConnection(connection, 'offline', 'Reconnecting…');
    showConnection(editorConnection, 'unknown', 'Unreal Editor · Unknown');
    if (displayedHistory === undefined) {
      placeholder.textContent = 'Could not load saves.';
      historyPlaceholder.textContent = 'Could not load saves.';
    }
    if (displayedEdits === undefined) editsPlaceholder.textContent = 'Could not load edits.';
    if (renderedRange === undefined) {
      dailyPlaceholder.textContent = failedActivity;
      topAssetsPlaceholder.textContent = failedActivity;
      for (const { placeholder } of assetLists) {
        placeholder.textContent = failedActivity;
      }
    }
  } finally {
    clearTimeout(timeout);
    if (activeRequest === controller) {
      activeRequest = undefined;
      refreshTimer = setTimeout(refreshActivity, refreshIntervalMs);
    }
  }
}

rangeControls.addEventListener('change', () => {
  showConnection(connection, 'updating', 'Updating range…');
  cancelAssetRequest();
  refreshActivity();
});

function dismissAssetDetail(restoreFocus = false) {
  if (selectedAsset === null) return;
  const { path, trigger, rows } = selectedAsset;
  selectedAsset = null;
  cancelAssetRequest();
  assetDetail.hidden = true;
  if (restoreFocus) {
    if (trigger.isConnected && !trigger.closest('[hidden], details:not([open])')) trigger.focus();
    else if (rows) restoreAssetFocus(path, rows);
    else pageTitle.focus();
  }
}

closeAssetDetail.addEventListener('click', () => dismissAssetDetail(true));
latestAssetReferences.addEventListener('click', () => {
  selectAsset(latestAssetReferences.dataset.packagePath, latestAssetReferences);
});
assetReferenceGraph.addEventListener('click', event => {
  const node = event.target.closest('button.reference-node:not(:disabled)');
  if (node) openAssetInEditor(node.dataset.packagePath);
});

function revealPage(page, path) {
  if (path === '/') {
    const cards = page.querySelectorAll('.overview-stat, .daily-card, .latest-card');
    cards.forEach((card, index) => reveal(card, { delay: index * 32, duration: 270 }));
    dailyGraph.onShow();
    return;
  }

  const cards = page.querySelectorAll('.activity-card');
  cards.forEach((card, index) => {
    const delay = index * 50;
    reveal(card, { delay, duration: 240 });
    revealRows([...card.querySelectorAll('tbody tr, .asset-folder > summary')], delay + 75);
  });
}

function showPage(path, focusHeading = false) {
  if (path === activePage) return;
  if (activePage === '/') dailyGraph.onHide();
  dismissAssetDetail();
  activePage = path;
  const currentPage = pages.get(path);
  for (const page of pages.values()) page.hidden = page !== currentPage;
  rangeControls.hidden = path === '/history' || path === '/edits';

  for (const link of sectionLinks) {
    if (link.pathname === path) link.setAttribute('aria-current', 'page');
    else link.removeAttribute('aria-current');
  }
  const currentLink = sectionLinks.find(link => link.pathname === path);
  const title = currentLink.textContent.trim();
  pageTitle.textContent = title;
  document.title = `${title} | Project Activity Visualizer`;

  window.scrollTo(0, 0);
  if (focusHeading) pageTitle.focus({ preventScroll: true });
  reveal(pageTitle, { distance: 2, duration: 170 });
  revealPage(currentPage, path);
}

function pageFromLocation() {
  const legacyPage = location.pathname === '/' ? oldPageLinks.get(location.hash) : undefined;
  if (legacyPage) history.replaceState(null, '', legacyPage + location.search);
  return legacyPage ?? (pages.has(location.pathname) ? location.pathname : '/');
}

document.addEventListener('click', event => {
  const link = event.target.closest('a[data-page-link]');
  if (!link || event.defaultPrevented || event.button !== 0
      || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
  event.preventDefault();
  const path = link.pathname;
  if (location.pathname !== path || location.hash) history.pushState(null, '', path);
  showPage(path, true);
});

history.scrollRestoration = 'manual';
window.addEventListener('popstate', () => showPage(pageFromLocation(), true));
showPage(pageFromLocation());
refreshActivity();
