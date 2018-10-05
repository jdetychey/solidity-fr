contract C {
  uint256 Test;

  function f() public {
    emit Test();
  }
}
// ----
// TypeError: (63-69): Type is not callable
