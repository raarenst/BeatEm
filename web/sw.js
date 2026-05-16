/* BeatEm service worker.
 *
 * Caches the static assets so the page can launch from the home screen
 * even when offline (the WebSocket itself obviously requires the
 * server, but the UI loads instantly). Intentionally minimal — no
 * push notifications (they'd defeat the protocol's privacy property)
 * and no background sync.
 *
 * Bump CACHE_NAME when index.html or assets change so old clients
 * fetch the new version on next visit.
 */
const CACHE_NAME = "beatem-v3";  /* bump when index.html / assets change */
const ASSETS = [
  "/",
  "/index.html",
  "/manifest.json",
  "/icon-192.png",
  "/icon-512.png",
  "/apple-touch-icon.png",
];

self.addEventListener("install", (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => cache.addAll(ASSETS))
  );
  self.skipWaiting();
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(
        keys.filter((k) => k !== CACHE_NAME).map((k) => caches.delete(k))
      )
    )
  );
  self.clients.claim();
});

self.addEventListener("fetch", (event) => {
  const req = event.request;
  if (req.method !== "GET") return;

  /* Network-first so updates show up immediately when online, falling
   * back to the cache when offline. */
  event.respondWith(
    fetch(req)
      .then((resp) => {
        if (resp && resp.status === 200) {
          const copy = resp.clone();
          caches.open(CACHE_NAME).then((c) => c.put(req, copy)).catch(() => {});
        }
        return resp;
      })
      .catch(() => caches.match(req))
  );
});
