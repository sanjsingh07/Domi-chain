import { expect } from "chai";
import { tocksToAnlog, TOCKS_PER_ANLOG } from "utils";
import BN from "bn.js";

describe("tocksToAnlog", () => {
  it("0 tocks", () => {
    expect(tocksToAnlog(new BN(0))).to.eq(0.0);
  });

  it("1 tock", () => {
    expect(tocksToAnlog(new BN(1))).to.eq(0.000000001);
    expect(tocksToAnlog(new BN(-1))).to.eq(-0.000000001);
  });

  it("1 ANLOG", () => {
    expect(tocksToAnlog(new BN(TOCKS_PER_ANLOG))).to.eq(1.0);
    expect(tocksToAnlog(new BN(-TOCKS_PER_ANLOG))).to.eq(-1.0);
  });

  it("u64::MAX tocks", () => {
    expect(tocksToAnlog(new BN(2).pow(new BN(64)))).to.eq(
      18446744073.709551615
    );
    expect(tocksToAnlog(new BN(2).pow(new BN(64)).neg())).to.eq(
      -18446744073.709551615
    );
  });
});
