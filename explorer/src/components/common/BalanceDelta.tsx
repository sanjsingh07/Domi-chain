import React from "react";
import { BigNumber } from "bignumber.js";
import { AnlogBalance } from "utils";

export function BalanceDelta({
  delta,
  isAnlog = false,
}: {
  delta: BigNumber;
  isAnlog?: boolean;
}) {
  let sols;

  if (isAnlog) {
    sols = <AnlogBalance tocks={delta.toNumber()} />;
  }

  if (delta.gt(0)) {
    return (
      <span className="badge badge-soft-success">
        +{isAnlog ? sols : delta.toString()}
      </span>
    );
  } else if (delta.lt(0)) {
    return (
      <span className="badge badge-soft-warning">
        {isAnlog ? <>-{sols}</> : delta.toString()}
      </span>
    );
  }

  return <span className="badge badge-soft-secondary">0</span>;
}
