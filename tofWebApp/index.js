(async function () {
  const setFrameTypeBtn = document.getElementById("setFrameType");
  const socket = new WebSocket("wss://localhost");

  setFrameTypeBtn.addEventListener("click", () => {
    socket.send("setFrameType");
    console.log("setFrameType");
  });

  const urlCreator = window.URL || window.webkitURL;
  socket.onmessage = async function (event) {
    const blob = await event.data.blob();
    var imageUrl = urlCreator.createObjectURL(blob);
    const imageViewer = document.getElementById("my-image");
    imageViewer.src = imageUrl;
  };
})();
