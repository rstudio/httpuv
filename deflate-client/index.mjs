import assert from "assert";
import child_process from "child_process";
import { randomBytes } from "crypto";
import { sleep, md5, WebSocketReceiver, withWS } from "./lib/util.mjs";

const MAX_BITS = 21;

async function runMD5Test(address, perMessageDeflate) {
  await withWS(address + "md5", { perMessageDeflate }, async (ws, wsr) => {
    process.stderr.write("   md5: ");
    for (let i = 0; i <= MAX_BITS; i++) {
      process.stderr.write(".");
      const payload = randomBytes(2**i);
      ws.send(payload);
      const { data } = await wsr.nextEvent("message");
      assert.strictEqual(data.toString("utf-8"), md5(payload), "md5 of payload must match server's response");    
    }
    process.stderr.write("\n");
  });
}

async function runEchoTest(address, perMessageDeflate) {
  await withWS(address + "echo", { perMessageDeflate }, async (ws, wsr) => {
    process.stderr.write("  echo: ");
    for (let i = 0; i <= MAX_BITS; i++) {
      process.stderr.write(".");
      const payload = randomBytes(2**i);
      ws.send(payload);
      const { data } = await wsr.nextEvent("message");
      assert(data.equals(payload), "payload must match server's response");    
    }
    process.stderr.write("\n");
  });
}

async function runTest(address, perMessageDeflate) {
  await runMD5Test(address, perMessageDeflate);
  await runEchoTest(address, perMessageDeflate);
}

async function main() {
  await sleep(1000);
  await runTest("ws://127.0.0.1:9100/", false);
  await runTest("ws://127.0.0.1:9100/", true);
  await runTest("ws://127.0.0.1:9100/", { threshold: 0 });
  await runTest("ws://127.0.0.1:9100/", {
    threshold: 0,
    serverMaxWindowBits: 9,
    clientMaxWindowBits: 9
  });
  await runTest("ws://127.0.0.1:9100/", {
    threshold: 0,
    serverMaxWindowBits: 9,
    clientMaxWindowBits: 9,
    serverNoContextTakeover: true
  });
}

console.error("Launching httpuv");
const rprocess = child_process.spawn("Rscript", ["server.R"], {
  stdio: ["inherit", "inherit", "inherit"]
});
process.on("exit", () => {
  rprocess.kill();
});

main().then(
  result => {
    console.error("All tests passed!");
    process.exit(0);
  },
  error => {
    console.error(error ?? "An unknown error has occurred!");
    process.exit(1);
  }
);
