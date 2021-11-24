import * as web3 from '@analog/web3.js';

(async () => {
  // Connect to cluster
  var connection = new web3.Connection(
    web3.clusterApiUrl('devnet'),
    'confirmed',
  );

  // Generate a new random public key
  var from = web3.Keypair.generate();
  var airdropSignature = await connection.requestAirdrop(
    from.publicKey,
    web3.TOCKS_PER_ANLOG,
  );
  await connection.confirmTransaction(airdropSignature);

  // Generate a new random public key
  var to = web3.Keypair.generate();

  // Add transfer instruction to transaction
  var transaction = new web3.Transaction().add(
    web3.SystemProgram.transfer({
      fromPubkey: from.publicKey,
      toPubkey: to.publicKey,
      tocks: web3.TOCKS_PER_ANLOG / 100,
    }),
  );

  // Sign transaction, broadcast, and confirm
  var signature = await web3.sendAndConfirmTransaction(
    connection,
    transaction,
    [from],
  );
  console.log('SIGNATURE', signature);
})();
