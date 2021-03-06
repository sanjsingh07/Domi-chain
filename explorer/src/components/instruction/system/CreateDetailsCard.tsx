import React from "react";
import {
  SystemProgram,
  SignatureResult,
  ParsedInstruction,
} from "@analog/web3.js";
import { AnlogBalance } from "utils";
import { InstructionCard } from "../InstructionCard";
import { Address } from "components/common/Address";
import { CreateAccountInfo } from "./types";

export function CreateDetailsCard(props: {
  ix: ParsedInstruction;
  index: number;
  result: SignatureResult;
  info: CreateAccountInfo;
  innerCards?: JSX.Element[];
  childIndex?: number;
}) {
  const { ix, index, result, info, innerCards, childIndex } = props;

  return (
    <InstructionCard
      ix={ix}
      index={index}
      result={result}
      title="Create Account"
      innerCards={innerCards}
      childIndex={childIndex}
    >
      <tr>
        <td>Program</td>
        <td className="text-lg-right">
          <Address pubkey={SystemProgram.programId} alignRight link />
        </td>
      </tr>

      <tr>
        <td>From Address</td>
        <td className="text-lg-right">
          <Address pubkey={info.source} alignRight link />
        </td>
      </tr>

      <tr>
        <td>New Address</td>
        <td className="text-lg-right">
          <Address pubkey={info.newAccount} alignRight link />
        </td>
      </tr>

      <tr>
        <td>Transfer Amount (GM)</td>
        <td className="text-lg-right">
          <AnlogBalance tocks={info.tocks} />
        </td>
      </tr>

      <tr>
        <td>Allocated Data Size</td>
        <td className="text-lg-right">{info.space} byte(s)</td>
      </tr>

      <tr>
        <td>Assigned Program Id</td>
        <td className="text-lg-right">
          <Address pubkey={info.owner} alignRight link />
        </td>
      </tr>
    </InstructionCard>
  );
}
