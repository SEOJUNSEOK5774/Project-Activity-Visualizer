export const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');

const easing = 'cubic-bezier(0.2, 0.7, 0.25, 1)';
const entrances = new WeakMap();
const lists = new WeakMap();

export function reveal(element, { distance = 4, delay = 0, duration = 240 } = {}) {
  entrances.get(element)?.cancel();
  if (reducedMotion.matches || !element.getClientRects().length) return;

  const animation = element.animate([
    { opacity: 0, transform: `translateY(${distance}px)` },
    { opacity: 1, transform: 'translateY(0)' },
  ], { delay, duration, easing, fill: 'backwards' });
  entrances.set(element, animation);
}

export function revealRows(rows, delay = 0) {
  if (reducedMotion.matches) return;
  let index = 0;
  for (const row of rows) {
    if (!row.getClientRects().length) continue;
    reveal(row, { distance: 3, delay: delay + index * 23, duration: 210 });
    index += 1;
    if (index === 8) break;
  }
}

function updateCells(row, replacement) {
  if (row.matches('tr')) {
    const nextCells = [...replacement.cells];
    [...row.cells].forEach((cell, index) => {
      if (!cell.isEqualNode(nextCells[index])) cell.replaceWith(nextCells[index]);
    });
  } else {
    row.replaceChildren(...replacement.children);
  }
}

export function updateList(container, items, getKey, createRow) {
  const previous = lists.get(container) ?? new Map();
  const next = new Map();
  const changed = [];
  const focused = container.contains(document.activeElement) ? document.activeElement : null;

  for (const item of items) {
    const key = getKey(item);
    const data = JSON.stringify(item);
    const existing = previous.get(key);
    let row = existing?.row;
    if (!row) {
      row = createRow(item);
      changed.push(row);
    } else if (existing.data !== data) {
      updateCells(row, createRow(item));
      changed.push(row);
    }
    next.set(key, { data, row });
  }

  for (const [key, { row }] of previous) {
    if (!next.has(key)) row.remove();
  }

  let cursor = container.firstElementChild;
  for (const { row } of next.values()) {
    if (row !== cursor) {
      if (row.parentNode === container && container.moveBefore) container.moveBefore(row, cursor);
      else container.insertBefore(row, cursor);
    }
    cursor = row.nextElementSibling;
  }
  lists.set(container, next);

  if (focused?.isConnected && focused !== document.activeElement) focused.focus({ preventScroll: true });
  revealRows(changed);
}
