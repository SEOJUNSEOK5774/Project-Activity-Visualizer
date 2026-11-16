import { reducedMotion, reveal } from './dashboard-motion.js';

const svgNamespace = 'http://www.w3.org/2000/svg';
const chartDateFormat = new Intl.DateTimeFormat('en-US', { dateStyle: 'medium', timeZone: 'UTC' });
const axisDateFormat = new Intl.DateTimeFormat('en-US', { month: 'short', day: 'numeric', timeZone: 'UTC' });
const metricLabels = { activityBucketCount: 'activity buckets', rawSaveCount: 'save events' };
const metricUnits = { activityBucketCount: 'bucket', rawSaveCount: 'save' };
const plotTop = 11;

function svgElement(tag, attributes, label) {
  const element = document.createElementNS(svgNamespace, tag);
  setAttributes(element, attributes);
  if (label !== undefined) element.textContent = label;
  return element;
}

function setAttributes(element, attributes) {
  for (const [name, value] of Object.entries(attributes)) element.setAttribute(name, value);
}

function curveThrough(points) {
  // Use the slopes of adjacent segments to keep each curve between its two values.
  const slopes = points.slice(1).map((point, index) => point.y - points[index].y);
  const tangents = points.map((_, index) => {
    if (index === 0) return slopes[0] ?? 0;
    if (index === points.length - 1) return slopes[index - 1];
    const before = slopes[index - 1];
    const after = slopes[index];
    return before * after > 0 ? 2 * before * after / (before + after) : 0;
  });

  let path = `M ${points[0].x} ${points[0].y}`;
  for (let index = 1; index < points.length; index += 1) {
    const start = points[index - 1];
    const end = points[index];
    const handle = (end.x - start.x) / 3;
    path += ` C ${start.x + handle} ${start.y + tangents[index - 1] / 3}`
      + ` ${end.x - handle} ${end.y - tangents[index] / 3} ${end.x} ${end.y}`;
  }
  return path;
}

export function createDailyChart(numberFormat) {
  const plot = document.getElementById('daily-plot');
  const svg = document.getElementById('daily-svg');
  const axes = document.getElementById('daily-axes');
  const line = document.getElementById('daily-line');
  const area = document.getElementById('daily-area');
  const previousPaths = [];
  const highlight = document.getElementById('daily-highlight');
  const guide = highlight.querySelector('line');
  const markers = highlight.querySelectorAll('circle');
  const tooltip = document.getElementById('daily-tooltip');
  const tooltipDate = tooltip.querySelector('time');
  const tooltipValue = tooltip.querySelector('strong');
  const announcement = document.getElementById('daily-chart-announcement');
  const metricControls = document.getElementById('daily-metric');

  let metric = metricControls.querySelector(':checked').value;
  let days = [];
  let points = [];
  let chartWidth;
  let chartHeight;
  let chartBottom;
  let activeIndex = -1;
  let keyboardInspecting = false;
  let motionFrame = 0;
  let pageVisible = false;

  function clearHighlight() {
    activeIndex = -1;
    keyboardInspecting = false;
    highlight.setAttribute('hidden', '');
    tooltip.hidden = true;
    if (announcement.textContent) announcement.textContent = '';
  }

  function positionHighlight(point) {
    setAttributes(guide, { x1: point.x, x2: point.x, y1: plotTop, y2: chartBottom });
    for (const marker of markers) setAttributes(marker, { cx: point.x, cy: point.y });

    const tipWidth = tooltip.offsetWidth;
    const tipHeight = tooltip.offsetHeight;
    const x = point.x + 12 + tipWidth <= chartWidth ? point.x + 12 : point.x - 12 - tipWidth;
    const y = point.y - tipHeight - 10 >= 4 ? point.y - tipHeight - 10 : point.y + 10;
    tooltip.style.translate = `${Math.max(0, Math.min(x, chartWidth - tipWidth))}px ${Math.max(0, Math.min(y, chartHeight - tipHeight))}px`;
  }

  function highlightDay(index, announce = false) {
    const point = points[index];
    if (!point) return;
    activeIndex = index;

    const day = days[index];
    const date = chartDateFormat.format(new Date(day.dateUtc));
    const count = day[metric];
    const value = `${numberFormat.format(count)} ${metricUnits[metric]}${count === 1 ? '' : 's'}`;
    tooltipDate.dateTime = day.dateUtc;
    if (tooltipDate.textContent !== date) tooltipDate.textContent = date;
    if (tooltipValue.textContent !== value) tooltipValue.textContent = value;

    const entering = tooltip.hidden;
    highlight.removeAttribute('hidden');
    tooltip.hidden = false;
    positionHighlight(point);
    if (entering) reveal(tooltip, { distance: 3, duration: 145 });

    const message = `${date}, ${value}`;
    if (announce && announcement.textContent !== message) announcement.textContent = message;
  }

  function cancelChartMotion() {
    cancelAnimationFrame(motionFrame);
    motionFrame = 0;
    for (const animation of [...line.getAnimations(), ...area.getAnimations()]) animation.cancel();
    for (const path of previousPaths) path.remove();
    previousPaths.length = 0;
  }

  function drawCurve(nextPoints) {
    points = nextPoints;
    const curve = curveThrough(points);
    const first = points[0];
    const last = points.at(-1);
    line.setAttribute('d', curve);
    area.setAttribute('d', `${curve} L ${last.x} ${chartBottom} L ${first.x} ${chartBottom} Z`);
    if (activeIndex >= 0) positionHighlight(points[activeIndex]);
  }

  function revealCurve() {
    line.animate(
      [{ strokeDasharray: '1', strokeDashoffset: '1' }, { strokeDasharray: '1', strokeDashoffset: '0' }],
      { delay: 75, duration: 650, easing: 'cubic-bezier(0.23, 0.65, 0.29, 1)', fill: 'backwards' },
    );
    area.animate([{ opacity: 0 }, { opacity: 1 }], {
      delay: 75, duration: 600, easing: 'ease-out', fill: 'backwards',
    });
  }

  function crossfadeCurve(nextPoints) {
    const layers = [area, line];
    for (const layer of layers) {
      const previous = layer.cloneNode();
      previous.removeAttribute('id');
      previousPaths.push(previous);
      layer.before(previous);
      previous.animate([{ opacity: 1 }, { opacity: 0 }], { duration: 220, easing: 'ease-out' })
        .onfinish = () => previous.remove();
      layer.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 320, easing: 'ease-out' });
    }
    drawCurve(nextPoints);
  }

  function moveCurve(nextPoints) {
    const start = points;
    const startedAt = performance.now();
    function frame(now) {
      const progress = Math.min(1, (now - startedAt) / 390);
      const eased = 1 - (1 - progress) ** 3;
      drawCurve(progress === 1 ? nextPoints : nextPoints.map((point, index) => ({
        x: start[index].x + (point.x - start[index].x) * eased,
        y: start[index].y + (point.y - start[index].y) * eased,
      })));
      motionFrame = progress < 1 ? requestAnimationFrame(frame) : 0;
    }
    motionFrame = requestAnimationFrame(frame);
  }

  function render(motion) {
    if (!pageVisible || days.length === 0) return;
    const width = plot.clientWidth;
    const height = plot.clientHeight;
    if (width === 0 || height === 0) return;
    if (!motion && width === chartWidth && height === chartHeight) return;

    const sameSize = width === chartWidth && height === chartHeight;
    chartWidth = width;
    chartHeight = height;
    cancelChartMotion();

    const maximum = Math.max(4, ...days.map(day => day[metric]));
    const magnitude = 10 ** Math.floor(Math.log10(maximum / 4));
    const step = [1, 2, 5, 10].map(value => value * magnitude).find(value => value >= maximum / 4);
    const ceiling = Math.ceil(maximum / step) * step;
    const left = Math.max(34, numberFormat.format(ceiling).length * 7 + 10);
    const right = width - 21;
    chartBottom = height - 24;
    const axisWidth = right - left;
    const yAt = value => chartBottom - value / ceiling * (chartBottom - plotTop);

    svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
    const axisElements = document.createDocumentFragment();
    for (let value = 0; value <= ceiling; value += step) {
      const y = yAt(value);
      axisElements.append(
        svgElement('line', { class: 'daily-grid-line', x1: left, x2: right, y1: y, y2: y }),
        svgElement('text', { x: left - 9, y: y + 3.5, 'text-anchor': 'end' }, numberFormat.format(value)),
      );
    }

    const lastDay = days.length - 1;
    const nextPoints = days.map((day, index) => ({
      x: left + index / Math.max(1, lastDay) * axisWidth,
      y: yAt(day[metric]),
    }));
    const labelCount = Math.min(days.length, 7, Math.max(2, Math.floor(axisWidth / 55) + 1));
    for (let index = 0; index < labelCount; index += 1) {
      const dayIndex = Math.round(index * lastDay / Math.max(1, labelCount - 1));
      const label = axisDateFormat.format(new Date(days[dayIndex].dateUtc));
      axisElements.append(svgElement(
        'text', { x: nextPoints[dayIndex].x, y: height - 5, 'text-anchor': 'middle' }, label,
      ));
    }
    axes.replaceChildren(axisElements);

    const animate = !reducedMotion.matches;
    if (motion === 'update' && sameSize && animate && points.length === nextPoints.length) {
      const hasMoved = points.some((point, index) => point.x !== nextPoints[index].x || point.y !== nextPoints[index].y);
      if (hasMoved) moveCurve(nextPoints);
    } else if (motion === 'range' && sameSize && animate && points.length) {
      crossfadeCurve(nextPoints);
    } else {
      drawCurve(nextPoints);
      if (motion === 'reveal' && animate) revealCurve();
    }

    if (activeIndex >= 0) highlightDay(activeIndex, keyboardInspecting);
  }

  function inspectPointer(event) {
    if (points.length === 0) return;
    const x = event.clientX - plot.getBoundingClientRect().left;
    const index = points.reduce((nearest, point, current) => (
      Math.abs(x - point.x) < Math.abs(x - points[nearest].x) ? current : nearest
    ), 0);
    keyboardInspecting = false;
    if (index !== activeIndex) highlightDay(index, event.pointerType === 'touch');
  }

  plot.addEventListener('pointermove', event => {
    if (event.pointerType !== 'touch') inspectPointer(event);
  });
  plot.addEventListener('pointerup', event => {
    if (event.pointerType === 'touch') inspectPointer(event);
  });
  document.addEventListener('pointerdown', event => {
    if (event.pointerType === 'touch' && !plot.contains(event.target)) clearHighlight();
  });
  plot.addEventListener('pointerleave', event => {
    if (event.pointerType !== 'touch' && !keyboardInspecting) clearHighlight();
  });
  plot.addEventListener('focus', () => {
    if (!plot.matches(':focus-visible') || points.length === 0) return;
    keyboardInspecting = true;
    highlightDay(points.length - 1, true);
  });
  plot.addEventListener('blur', clearHighlight);
  plot.addEventListener('keydown', event => {
    const last = points.length - 1;
    if (last < 0) return;
    const current = activeIndex >= 0 ? activeIndex : last;
    let index;
    switch (event.key) {
      case 'Escape':
        clearHighlight();
        return;
      case 'ArrowLeft':
        index = Math.max(0, current - 1);
        break;
      case 'ArrowRight':
        index = Math.min(last, current + 1);
        break;
      case 'Home':
        index = 0;
        break;
      case 'End':
        index = last;
        break;
      default:
        return;
    }
    event.preventDefault();
    keyboardInspecting = true;
    highlightDay(index, true);
  });

  metricControls.addEventListener('change', event => {
    metric = event.target.value;
    plot.setAttribute('aria-label', `Daily ${metricLabels[metric]} chart`);
    clearHighlight();
    render('update');
  });

  new ResizeObserver(() => render()).observe(plot);
  reducedMotion.addEventListener('change', () => render('update'));
  return {
    onShow() {
      pageVisible = true;
      render('reveal');
    },
    onHide() {
      pageVisible = false;
      clearHighlight();
      cancelChartMotion();
    },
    setData(nextDays, revealNewRange) {
      const activeDate = days[activeIndex]?.dateUtc;
      days = nextDays;
      activeIndex = nextDays.findIndex(day => day.dateUtc === activeDate);
      if (activeIndex === -1) clearHighlight();
      const motion = revealNewRange ? (points.length ? 'range' : 'reveal') : 'update';
      render(motion);
    },
  };
}
