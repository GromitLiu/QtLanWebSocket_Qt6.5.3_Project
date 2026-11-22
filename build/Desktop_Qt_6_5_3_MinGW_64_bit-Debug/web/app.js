(() => {
  const $ = id => document.getElementById(id);
  const status = $("status"), serverInput = $("server");
  let socket = null;

  // 网页由 Qt 服务端提供时，默认自动使用当前页面主机 IP。
  serverInput.value = location.hostname || "127.0.0.1";

  function setConnected(connected, text) {
    status.textContent = text || (connected ? "已连接" : "未连接");
    status.className = connected ? "online" : "offline";
    $("connect").disabled = connected;
    $("disconnect").disabled = !connected;
    $("send").disabled = !connected;
  }
  function addLog(title, content) {
    const empty = document.querySelector(".empty");
    if (empty) empty.remove();
    const entry = document.createElement("div");
    entry.className = "entry";
    const time = document.createElement("small");
    time.textContent = new Date().toLocaleTimeString() + " · " + title;
    const body = document.createElement("div");
    body.textContent = content;
    entry.append(time, body);
    $("log").prepend(entry);
  }
  function connect() {
    const host = serverInput.value.trim();
    if (!host) { addLog("错误", "请输入服务器 IP 地址"); return; }
    if (socket) { socket.close(); socket = null; }
    const url = `ws://${host}:8888`;
    setConnected(false, "正在连接...");
    addLog("系统", "正在连接 " + url);
    try {
      socket = new WebSocket(url);
      socket.onopen = () => { setConnected(true); addLog("系统", "WebSocket 连接成功"); };
      socket.onmessage = event => {
        let content = event.data;
        try {
          const obj = JSON.parse(event.data);
          content = obj.content || obj.message || event.data;
        } catch (_) {}
        addLog("服务器消息", content);
      };
      socket.onerror = () => addLog("错误", "连接发生错误，请检查服务器、IP、防火墙和 Wi-Fi 网络隔离设置");
      socket.onclose = () => { setConnected(false); addLog("系统", "连接已关闭"); };
    } catch (error) {
      setConnected(false);
      addLog("错误", String(error));
    }
  }
  $("connect").addEventListener("click", connect);
  $("disconnect").addEventListener("click", () => { if (socket) socket.close(); });
  $("send").addEventListener("click", () => {
    const value = $("message").value.trim();
    if (!value || !socket || socket.readyState !== WebSocket.OPEN) return;
    socket.send(value);
    addLog("我发送", value);
    $("message").value = "";
  });
  $("message").addEventListener("keydown", event => {
    if (event.key === "Enter" && !event.shiftKey) { event.preventDefault(); $("send").click(); }
  });
  $("clear").addEventListener("click", () => { $("log").innerHTML = '<p class="empty">暂无通信记录。</p>'; });
  // 自动连接：网页加载后稍作停顿，便于用户看到状态。
  setTimeout(connect, 350);
})();
